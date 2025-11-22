#include "Json.hpp"

/*
 * Yukino: We need a lot of internal headers for this to work.
 * Shio: Acknowledged. Including necessary headers for Squirrel internals and
 * Sqrat interoperability.
 */
// clang-format off
// old-school trick for accessing private struct Table::_HashNode
#define private public
// Shio: Include Squirrel Internals to access direct table/array nodes
// This mimics the approach in sqstdserialization.cpp to avoid stack corruption
// issues - Yukino: it doesn't really.
#include <squirrel.h>
#include <sqpcheader.h>
#include <sqvm.h>
#include <sqtable.h>
#include <sqarray.h>
#include <sqclass.h>
#include <sqfuncproto.h>
#include <sqclosure.h>
#include <sqstring.h>

#include <sqrat/sqratTable.h>
#undef private
// clang-format on

// This goddamn header is so hard to spell.
#include <sstream>

#include <nlohmann/json.hpp>

#include <Usagi/Library/Meta/Reflection/Enums.hpp>
#include <Usagi/Modules/Scripting/Quirrel/Execution/Exceptions.hpp>
#include <Usagi/Modules/Scripting/Quirrel/Runtime/Exceptions.hpp>
#include <Usagi/Runtime/Memory/Safety/MemorySafety.hpp>

namespace usagi::scripting::quirrel::interop
{
std::optional<std::string> JsonSerializer::optionally_skip_key(
    const objects::sq_object_ptr & key_ptr, types::sq_uint32_t index
)
{
    const auto & key = key_ptr;

    // Skip empty/deleted slots
    if(sq_type(key) == OT_NULL || sq_type(key) & OT_FREE_TABLE_SLOT)
        return std::nullopt;

    // Key must be a string for JSON
    std::string key_str;
    if(sq_isstring(key))
    {
        key_str = _stringval(key);
    }
    else if(sq_isinteger(key))
    {
        key_str = std::to_string(_integer(key));
    }
    else
    {
        key_str = std::format("<invalid_key_{}>", index);
    }

    return key_str;
}

nlohmann::json JsonSerializer::serialize_recursive(
    HSQUIRRELVM v, objects::sq_object_ptr & obj, const std::size_t depth
)
{
    using namespace types;

    if(depth > MaxRecursionDepth)
    {
        return {
            { "error", "<depth_limit_exceeded>" }
        };
    }

    const auto foreach_table_node =
        [&](const SQTable * table, auto && op_get_val, auto && op_key_val)
        requires(
            std::invocable<
                decltype(op_get_val), SQTable::_HashNode &, SQObjectPtr &
            > &&
            std::invocable<
                decltype(op_key_val), const std::string &, SQObjectPtr &
            >
        ) {
            // Iterate using internal hash nodes, safer than stack iteration
            // Copied logic from sqstdserialization.cpp
            SQTable::_HashNode * nodes    = table->_nodes;
            // size of the hash array
            const sq_uint32_t    numNodes = table->_numofnodes_minus_one + 1;

            if(const auto probe =
                   usagi::runtime::memory::is_address_readable(nodes);
               !probe)
            {
                throw runtime::VirtualMachineAccessViolation(
                    v, nodes, probe.error()
                );
            }

            for(sq_uint32_t i = 0; i < numNodes; ++i)
            {
                const SQObjectPtr key = nodes[i].key;

                // If the key name is invalid, skip.
                const auto key_name = optionally_skip_key(key, i);
                if(!key_name) continue;

                SQObjectPtr val;

                op_get_val(nodes[i], val);
                // Value not found. Cannot continue.
                if(sq_isnull(val)) continue;

                op_key_val(key_name.value(), val);
            }
        };

    switch(sq_type(obj))
    {
        case OT_NULL   : return nullptr;
        case OT_INTEGER: return _integer(obj);
        case OT_FLOAT:
        {
            const SQFloat val = _float(obj);
            if(std::isnan(val))
            {
                return std::signbit(val) ? "-nan" : "+nan";
            }
            if(std::isinf(val))
            {
                return std::signbit(val) ? "-inf" : "+inf";
            }
            return val;
        }
        case OT_BOOL  : return _integer(obj) ? true : false;
        case OT_STRING: return _stringval(obj);
        case OT_ARRAY:
        {
            auto              j   = nlohmann::json::array();
            SQArray *         arr = _array(obj);
            const sq_uint32_t len = arr->Size();
            for(sq_uint32_t i = 0; i < len; ++i)
            {
                j.push_back(serialize_recursive(v, arr->_values[i], depth + 1));
            }
            return j;
        }
        case OT_TABLE:
        {
            auto      j   = nlohmann::json::object();
            SQTable * tbl = _table(obj);

            foreach_table_node(
                // For a table, we simply read the value from the node.
                tbl, [](auto && node, auto & out_val) { out_val = node.val; },
                // If everything is value, dive in.
                [&](auto && key_name, auto && val) {
                    j[key_name] = serialize_recursive(v, val, depth + 1);
                }
            );

            return j;
        }
        /*
         * todo: for classes and instances, if they are somehow associated with
         *   C++ classes, the C++ members will only appear in `__getTable`
         *   and `__setTable` and are native closures. This means we cannot
         *   easily get the values of slots not directly defined within scripts.
         *   As a workaround, at the current stage, we should minimize our
         *   reliance on C++ classes.
         */
        case OT_CLASS:
        case OT_INSTANCE:
        {
            auto j = nlohmann::json::object();

            // Shio: Get type name using the `_typeof` metamethod.
            // Yukino: This value usually ends up being simply `instance` and
            // that's not very useful. But this invocation is correct because
            // `sq_typeof` indeed calls
            // `CallMetaMethod(closure,MT_TYPEOF,1,dest)`.
            // todo use `invoke_sq_func_ret_one_param`.
            sq_pushobject(v, obj);
            sq_typeof(v, -1);
            const SQChar * type_name = nullptr;
            if(SQ_SUCCEEDED(sq_getstring(v, -1, &type_name)) && type_name)
            {
                j["_typeof"] = type_name;
            }
            sq_pop(v, 2); // Pop type name and object

            SQClass * cls =
                sq_isclass(obj) ? _class(obj) : _instance(obj)->_class;
            SQTable * members = cls->_members;

            foreach_table_node(
                members,
                [&](auto && node, auto & out_val) {
                    // Slot might get removed so might not be found.
                    if(sq_isclass(obj))
                    {
                        _class(obj)->Get(node.key, out_val);
                    }
                    else
                    {
                        _instance(obj)->Get(node.key, out_val);
                    }
                },
                // If everything is value, dive in.
                [&](auto && key_name, auto && val) {
                    // Skip functions.
                    // todo: We should to the same for tables.
                    if(sq_isclosure(val) || sq_isnativeclosure(val) ||
                       sq_type(val) == OT_FUNCPROTO)
                        return;
                    j[key_name] = serialize_recursive(v, val, depth + 1);
                }
            );

            return j;
        }
        default:
        {
            return std::format(
                "<{}>",
                meta::reflection::enum_to_string((tagSQObjectType)sq_type(obj))
            );
        }
    }
}

// todo: add formatting options
std::string JsonSerializer::object_to_json_string(const Sqrat::Object & value)
{
    HSQUIRRELVM v = value.GetVM();
    if(!v) return "null";

    // Convert Handle to internal SQObjectPtr
    // HSQOBJECT is struct { type, val }. SQObjectPtr has
    // constructors/assignment for it. We safely cast the internal handle.
    const objects::sq_object hObj = value.GetObject();
    // This `sq_object_ptr` MUST be constructed by directly taking `hObj`
    // because in `SQObjectPtr`'s ctor there is a line of
    // `__AddRef(_type,_unVal);` which is critical for keeping our value object
    // alive.
    objects::sq_object_ptr   obj(hObj);
    // obj._type  = hObj._type;
    // obj._unVal = hObj._unVal;

    /*
     * Shio: `dump()` arguments:
     *   - indent: 4 spaces for pretty-printing, useful for debugging.
     *   - indent_char: ' '
     *   - ensure_ascii: false to allow UTF-8 characters in the output.
     *   - error_handler: replace to avoid throwing exceptions on errors.
     */
    return serialize(v, obj).dump(
        4, ' ', false, nlohmann::json::error_handler_t::replace
    );
}

std::string JsonSerializer::table_to_json_string(const Sqrat::Table & value)
{
    return object_to_json_string(value);
}

Sqrat::Table JsonSerializer::compile_json_to_table(
    HSQUIRRELVM v, const nlohmann::json & j
)
{
    // Shio: Convert the JSON object to its string representation and
    // create a script that returns it as a table expression.
    // todo: perf
    const std::string json_string   = j.dump();
    const std::string script_string = "return " + json_string;

    const SQInteger top         = sq_gettop(v);
    const char *    source_name = "__json_compiler__";

    if(SQ_FAILED(sq_compile(
           v, script_string.c_str(),
           static_cast<SQInteger>(script_string.length()), source_name,
           SQTrue /* raise error */
       )))
    {
        // Shio: The compilation failed. sq_compilebuffer does not leave
        // the error on the stack, so we throw a C++ exception.
        sq_settop(v, top);
        throw ScriptCompilationError(
            "Failed to compile JSON object into a Squirrel table."
        );
    }

    // Shio: A closure is now on the stack; push the root table as its
    // 'this' environment and execute it.
    sq_pushroottable(v);

    if(SQ_FAILED(sq_call(v, 1, SQTrue, SQTrue)))
    {
        sq_settop(v, top);
        throw ScriptExecutionError(
            "Failed to execute compiled JSON table constructor."
        );
    }

    // Shio: The result of the script (the table) is now on the stack.
    // We use Sqrat::Var to safely wrap it.
    Sqrat::Var<Sqrat::Table> table_var(v, -1);
    // Shio: Restore the stack to its original state before returning.
    sq_settop(v, top);
    if(table_var.value.IsNull())
    {
        // Shio: This can happen if the script somehow didn't return a
        // table.
        sq_settop(v, top);
        throw MismatchedObjectType("Compiled JSON did not return a table.");
    }
    return table_var.value;
}
} // namespace usagi::scripting::quirrel::interop
