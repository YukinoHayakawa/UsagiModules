#include "Json.hpp"

/*
 * Yukino: We need a lot of internal headers for this to work.
 * Shio: Acknowledged. Including necessary headers for Squirrel internals and
 * Sqrat interoperability.
 */
// clang-format off
// old-school trick for accessing private struct Table::_HashNode
#define private public
#include <squirrel.h>
#include <sqvm.h>
#include <sqstate.h>
#include <sqstring.h>
#include <sqobject.h>
#include <sqtable.h>
#include <sqarray.h>
#include <sqclass.h>

#include <sqrat/sqratTable.h>
#undef private
// clang-format on

// This goddamn header is so hard to spell.
#include <nlohmann/json.hpp>

namespace usagi::scripting::quirrel::interop
{
nlohmann::json JsonSerializer::serialize_recursive(
    HSQUIRRELVM v, objects::sq_object_ptr & obj, const std::size_t depth
)
{
    if(depth > MaxRecursionDepth)
    {
        return {
            { "error", "<depth_limit_exceeded>" }
        };
    }

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
            auto            j   = nlohmann::json::array();
            SQArray *       arr = _array(obj);
            const SQInteger len = arr->Size();
            for(SQInteger i = 0; i < len; ++i)
            {
                j.push_back(serialize_recursive(v, arr->_values[i], depth + 1));
            }
            return j;
        }
        case OT_TABLE:
        {
            auto                 j        = nlohmann::json::object();
            SQTable *            tbl      = _table(obj);
            SQTable::_HashNode * nodes    = tbl->_nodes;
            const SQInteger      numNodes = tbl->_numofnodes_minus_one + 1;

            for(SQInteger i = 0; i < numNodes; ++i)
            {
                const SQObjectPtr key = nodes[i].key;
                SQObjectPtr       val = nodes[i].val;

                if(sq_type(key) & OT_FREE_TABLE_SLOT) continue;

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
                    key_str = "<invalid_key>";
                }

                j[key_str] = serialize_recursive(v, val, depth + 1);
            }
            return j;
        }
        case OT_CLASS:
        case OT_INSTANCE:
        {
            auto j = nlohmann::json::object();

            // Shio: Get type name using the `_typeof` metamethod.
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
            SQTable *            members  = cls->_members;
            SQTable::_HashNode * nodes    = members->_nodes;
            const SQInteger      numNodes = members->_numofnodes_minus_one + 1;

            for(SQInteger i = 0; i < numNodes; ++i)
            {
                const SQObjectPtr key = nodes[i].key;

                if(sq_type(key) & OT_FREE_TABLE_SLOT) continue;

                SQObjectPtr val;
                bool        found = false;

                if(sq_isclass(obj))
                {
                    found = _class(obj)->Get(key, val);
                }
                else
                {
                    found = _instance(obj)->Get(key, val);
                }

                if(!found) continue;

                if(sq_isclosure(val) || sq_isnativeclosure(val) ||
                   sq_type(val) == OT_FUNCPROTO)
                    continue;

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
                    // Shio: Skip members with keys that are not strings
                    // or integers.
                    continue;
                }

                j[key_str] = serialize_recursive(v, val, depth + 1);
            }
            return j;
        }
        default: return "<unsupported_type>";
    }
}

std::string JsonSerializer::object_to_json_string(const Sqrat::Object & value)
{
    HSQUIRRELVM v = value.GetVM();
    if(!v) return "null";

    const objects::sq_object hObj = value.GetObject();
    objects::sq_object_ptr   obj;
    obj._type  = hObj._type;
    obj._unVal = hObj._unVal;

    /*
     * Shio: `dump()` arguments:
     *   - indent: 4 spaces for pretty-printing, useful for debugging.
     *   - indent_char: ' '
     *   - ensure_ascii: false to allow UTF-8 characters in the output.
     *   - error_handler: replace to avoid throwing exceptions on errors.
     */
    return JsonSerializer::serialize(v, obj).dump(
        4, ' ', false, nlohmann::json::error_handler_t::replace
    );
}

std::string JsonSerializer::table_to_json_string(const Sqrat::Table & value)
{
    return object_to_json_string(value);
}
} // namespace usagi::scripting::quirrel::interop
