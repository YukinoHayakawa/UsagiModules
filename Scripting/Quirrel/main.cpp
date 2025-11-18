#include <cassert>
#include <cmath>
#include <iomanip>
#include <sstream>

// clang-format off
#include <squirrel.h>
#include <sqobject.h>
#include <sqvm.h>

// Include Squirrel Internals to access direct table/array nodes
// This mimics the approach in sqstdserialization.cpp to avoid stack corruption issues
#include <squirrel.h>
#include <squirrel/sqpcheader.h>
#include <squirrel/sqvm.h>
#include <squirrel/sqtable.h>
#include <squirrel/sqarray.h>
#include <squirrel/sqclass.h>
#include <squirrel/sqfuncproto.h>
#include <squirrel/sqclosure.h>
#include <squirrel/sqstring.h>
// clang-format on

#include <Usagi/Modules/Runtime/Logging/RuntimeLogger.hpp>
#include <Usagi/Runtime/Services/SimpleServiceProvider.hpp>

#include "Execution/VirtualMachines/VirtualMachine.hpp"

using namespace usagi::scripting::quirrel;

VirtualMachine * gGameServer = nullptr;

/**
 * @brief Represents a C++ object that can be manipulated by Quirrel.
 */
class GameObject
{
public:
    int   id;
    float x, y, z;

    GameObject(int objectId) : id(objectId), x(0.0f), y(0.0f), z(0.0f)
    {
        // Shio: [C++] GameObject created.
        gGameServer->logger().info("[C++] GameObject(id={}) created.", id);
    }

    void SetPosition(float nx, float ny, float nz)
    {
        x = nx;
        y = ny;
        z = nz;
        // Shio: [C++] GameObject position set.
        gGameServer->logger().info(
            "[C++] GameObject {} position set to ({}, {}, {})", id, x, y, z
        );
    }
};

float get_delta_time()
{
    gGameServer->logger().info("get_delta_time() hit");
    return 0.016f; // Stub: always return 16ms
}

SQInteger native_log(HSQUIRRELVM v)
{
    const SQChar * str;
    SQInteger      size;
    if(SQ_SUCCEEDED(sq_getstringandsize(v, 2, &str, &size)))
    {
        // Shio: native_log
        gGameServer->logger().info(" {}", std::string_view(str, size));
        // sq_pop(v, 2);
    }
    return 0; // 0 return values
}

// Helper to escape JSON strings
static std::string JsonEscape(const char * str)
{
    if(!str) return "\"\"";

    std::stringstream ss;
    ss << "\"";
    for(const char * c = str; *c; ++c)
    {
        switch(*c)
        {
            case '"' : ss << "\\\""; break;
            case '\\': ss << "\\\\"; break;
            case '\b': ss << "\\b"; break;
            case '\f': ss << "\\f"; break;
            case '\n': ss << "\\n"; break;
            case '\r': ss << "\\r"; break;
            case '\t': ss << "\\t"; break;
            default:
                if(static_cast<unsigned char>(*c) < 0x20)
                {
                    ss << "\\u" << std::setfill('0') << std::setw(4) << std::hex
                       << static_cast<int>(*c);
                }
                else
                {
                    ss << *c;
                }
        }
    }
    ss << "\"";
    return ss.str();
}

struct SQJsonSerializer
{
    enum
    {
        MAX_DEPTH = 200,
    };

    std::stringstream & ss;
    uint32_t            depth;
    bool                success;

    SQJsonSerializer(std::stringstream & stream)
        : ss(stream)
        , depth(0)
        , success(true)
    {
    }

    bool serialize(SQObjectPtr obj)
    {
        if(depth > MAX_DEPTH)
        {
            // Fallback or error
            ss << "\"<depth_limit_exceeded>\"";
            return false;
        }

        switch(sq_type(obj))
        {
            case OT_NULL   : ss << "null"; break;
            case OT_INTEGER: ss << _integer(obj); break;
            case OT_FLOAT:
            {
                SQFloat val = _float(obj);
                if(std::isnan(val) || std::isinf(val))
                {
                    ss << "null";
                }
                else
                {
                    ss << val;
                }
                break;
            }
            case OT_BOOL  : ss << (_integer(obj) ? "true" : "false"); break;
            case OT_STRING: ss << JsonEscape(_stringval(obj)); break;
            case OT_ARRAY:
            {
                ss << "[";
                SQArray * arr = _array(obj);
                SQInteger len = arr->Size();
                depth++;
                for(SQInteger i = 0; i < len; ++i)
                {
                    if(i > 0) ss << ",";
                    // Direct internal access to array values
                    serialize(arr->_values[i]);
                }
                depth--;
                ss << "]";
                break;
            }
            case OT_TABLE:
            {
                ss << "{";
                SQTable * tbl = _table(obj);
                depth++;

                // Iterate using internal hash nodes, safer than stack iteration
                // Copied logic from sqstdserialization.cpp
                SQTable::_HashNode * nodes = tbl->_nodes;
                SQInteger            numNodes =
                    tbl->_numofnodes_minus_one + 1; // size of the hash array

                bool first = true;
                for(SQInteger i = 0; i < numNodes; ++i)
                {
                    SQObjectPtr key = nodes[i].key;
                    SQObjectPtr val = nodes[i].val;

                    // Skip empty/deleted slots
                    if(sq_type(key) == OT_NULL ||
                       (sq_type(key) & OT_FREE_TABLE_SLOT))
                        continue;

                    if(!first) ss << ",";

                    // Key must be a string for JSON
                    if(sq_isstring(key))
                    {
                        ss << JsonEscape(_stringval(key));
                    }
                    else
                    {
                        // Convert non-string key to string (basic conversion)
                        if(sq_isinteger(key))
                        {
                            ss << "\"" << _integer(key) << "\"";
                        }
                        else
                        {
                            ss << "\"<invalid_key>\"";
                        }
                    }

                    ss << ":";
                    serialize(val);
                    first = false;
                }
                depth--;
                ss << "}";
                break;
            }
            case OT_CLASS:
            {
                // Serialize class default values as a JSON object or just
                // placeholder sqstdserialization.cpp serializes classes via
                // specific logic, here we just output a placeholder as JSON
                // usually represents data, not schema.
                ss << "\"<class>\"";
                break;
            }
            case OT_INSTANCE:
            {
                // For instances, we usually want to serialize properties.
                // Without __getstate/JSON support in the class, this is
                // ambiguous. We'll placeholder it for safety unless we
                // implement full __getstate support.
                ss << "\"<instance>\"";
                break;
            }
            default: ss << "\"<unsupported_type>\""; break;
        }
        return true;
    }
};

std::string to_json_string_(const Sqrat::Object & value)
{
    HSQUIRRELVM v = value.GetVM();
    if(!v) return "null";

    std::stringstream ss;
    SQJsonSerializer  serializer(ss);

    // Convert Handle to internal SQObjectPtr
    // HSQOBJECT is struct { type, val }. SQObjectPtr has
    // constructors/assignment for it. We safely cast the internal handle.
    HSQOBJECT   hObj = value.GetObject();
    SQObjectPtr obj;
    obj._type  = hObj._type;
    obj._unVal = hObj._unVal;

    serializer.serialize(obj);

    return ss.str();
}

std::string to_json_string(const Sqrat::Table & value)
{
    return to_json_string_(static_cast<const Sqrat::Object &>(value));
}

// see `VarTrace::saveStack` todo this is a TEMP test
std::string to_string(/*HSQUIRRELVM vm, */ const Sqrat::Table & value)
{
    SQObjectPtr obj(value.GetObject());
    SQObjectPtr res;
    if(gGameServer->get_vm()->ToString(obj, res))
    {
        const SQChar * valueAsString = sq_objtostring(&res);
        if(valueAsString)
        {
            gGameServer->logger().info(" {}", std::string_view(valueAsString));
        }
        return valueAsString;
    }
    return "<to_string() error>";
}

using namespace usagi::runtime;

/**
 * @brief Main function to run the server example.
 */
int main(int argc, char ** argv)
{
    // Create the dummy script files on disk
    // CreateDummyScripts();

    auto env = std::make_shared<RuntimeEnvironment>();

    // Add logging service.
    env->service_provider.create_default_service<RuntimeLogger>()
        .value()
        .get()
        .add_console_sink();

    auto & root_vm = env->service_provider
                         .create_service(std::make_unique<VirtualMachine>(env))
                         .value()
                         .get();

    gGameServer = &root_vm;

    root_vm.init();
    root_vm.RegisterCommandLineArgs(argc, argv);

    {
        // 1. Create an 'exports' table for our native module
        auto exports = root_vm.CreateBindNewObject<Sqrat::Table>();

        // 2. Bind C++ functions
        // Use SquirrelFuncDeclString for full type-hinting and docs
        /*
        exports.SquirrelFuncDeclString(
            // auto derive function type
            Sqrat::SqGlobalThunk<decltype(&get_delta_time)>(),
            "get_delta_time(): float",
            "Returns engine delta time (fixed at 0.016)");
        */
        exports.Func(
            // auto derive function type
            "get_delta_time", &get_delta_time,
            "Returns engine delta time (fixed at 0.016)"
        );
        // exports.Func("native_log", &native_log);
        // `native_log` is a motherfucking raw Squirrel function instead of a
        // C++ one.
        // exports.SquirrelFunc("native_log", &native_log, -1);
        // This is the modern, declarative way to bind a native C-style function
        exports.SquirrelFuncDeclString(
            &native_log, // 1. The C++ function pointer (must be an SQFUNCTION)
            "native_log(s: string): null",     // 2. The Quirrel function
                                               // declaration string
            "Logs a string to the C++ console" // 3. The optional docstring
        );

        exports.Func("to_json_string", &to_json_string);

        // 3. Bind the C++ GameObject class
        auto gameObjClass =
            root_vm.CreateBindNewObject<Sqrat::Class<GameObject>>("GameObject");
        // Sqrat::Class<GameObject> gameObjClass(v, "GameObject");
        gameObjClass
            .Ctor<int>()              // Bind constructor: GameObject(int id)
            .Var("x", &GameObject::x) // Bind member variable
            .Var("y", &GameObject::y)
            .Var("z", &GameObject::z)
            .Func(
                "SetPosition",
                &GameObject::SetPosition
            ); // Bind member function

        // 4. Bind the class itself to the exports table
        exports.Bind("GameObject", gameObjClass);

        // 5. Register our populated exports table as a native module
        root_vm.module_manager().addNativeModule("engine_core", exports);
    }

    /*
    if(!server.init())
    {
        return 1;
    }
    */

    if(!root_vm.loadScripts())
    {
        return 1;
    }

    // Shio: --- SCRIPT SERVER RUNNING ---
    root_vm.logger().info("\n--- SCRIPT SERVER RUNNING ---");
    // Shio: --- Ticking 500 frames... ---
    root_vm.logger().info("--- Ticking 500 frames... ---");
    // Shio: --- Press to tick 100 frames (R+ENTER to reload) ---
    root_vm.logger().info(
        "--- Press to tick 100 frames (R+ENTER to reload) ---"
    );

    int frame = 0;
    while(frame < 100)
    {
        if(frame == 50)
        {
            // Trigger a hot-reload mid-flight
            root_vm.logger().info("TRIGGERING HOT-RELOAD");
            root_vm.triggerReload();
        }

        root_vm.tick();
        frame++;
    }

    // Shio: --- SIMULATION FINISHED ---
    root_vm.logger().info("\n--- SIMULATION FINISHED ---");
    root_vm.shutdown();
    return 0;
}
