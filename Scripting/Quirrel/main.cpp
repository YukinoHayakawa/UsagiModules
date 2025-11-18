#include <spdlog/spdlog.h>

#include <Usagi/Modules/Runtime/Logging/RuntimeLogger.hpp>
#include <Usagi/Runtime/Services/SimpleServiceProvider.hpp>

#include "Execution/VirtualMachines/VirtualMachine.hpp"

using namespace usagi::scripting::quirrel;

namespace
{
std::shared_ptr<VirtualMachine> gGameServer;

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
} // namespace

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
