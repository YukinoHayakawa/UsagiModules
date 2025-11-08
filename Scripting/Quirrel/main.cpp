#include <iostream>

#include <spdlog/spdlog.h>

#include "Runtime/VirtualMachine.hpp"

using namespace usagi::scripting::quirrel;

namespace
{
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
        std::cout << "[C++] GameObject(id=" << id << ") created." << std::endl;
    }

    void SetPosition(float nx, float ny, float nz)
    {
        x = nx;
        y = ny;
        z = nz;
        std::cout << "[C++] GameObject " << id << " position set to (" << x
                  << ", " << y << ", " << z << ")" << std::endl;
    }
};

float get_delta_time()
{
    return 0.016f; // Stub: always return 16ms
}

SQInteger native_log(HSQUIRRELVM v)
{
    const SQChar * str;
    if(SQ_SUCCEEDED(sq_getstring(v, 2, &str)))
    {
        std::cout << " " << str << std::endl;
    }
    return 0; // 0 return values
}
} // namespace

/**
 * @brief Main function to run the server example.
 */
int main(int argc, char ** argv)
{
    // Create the dummy script files on disk
    // CreateDummyScripts();

    auto server = std::make_shared<VirtualMachine>();

    server->init();
    server->RegisterCommandLineArgs(argc, argv);

    {
        // 1. Create an 'exports' table for our native module
        auto exports = server->CreateBindNewObject<Sqrat::Table>();

        // 2. Bind C++ functions
        // Use SquirrelFuncDeclString for full type-hinting and docs
        exports.SquirrelFuncDeclString(
            // auto derive function type
            Sqrat::SqGlobalThunk<decltype(&get_delta_time)>(),
            "get_delta_time(): float",
            "Returns engine delta time (fixed at 0.016)");
        exports.Func("native_log", &native_log);

        // 3. Bind the C++ GameObject class
        auto gameObjClass =
            server->CreateBindNewObject<Sqrat::Class<GameObject>>("GameObject");
        // Sqrat::Class<GameObject> gameObjClass(v, "GameObject");
        gameObjClass
            .Ctor<int>()              // Bind constructor: GameObject(int id)
            .Var("x", &GameObject::x) // Bind member variable
            .Var("y", &GameObject::y)
            .Var("z", &GameObject::z)
            .Func(
                "SetPosition",
                &GameObject::SetPosition); // Bind member function

        // 4. Bind the class itself to the exports table
        exports.Bind("GameObject", gameObjClass);

        // 5. Register our populated exports table as a native module
        server->module_manager().addNativeModule("engine_core", exports);
    }

    /*
    if(!server.init())
    {
        return 1;
    }
    */

    if(!server->loadScripts())
    {
        return 1;
    }

    std::cout << "\n--- SCRIPT SERVER RUNNING ---" << std::endl;
    std::cout << "--- Ticking 500 frames... ---" << std::endl;
    std::cout << "--- Press to tick 100 frames (R+ENTER to reload) ---"
              << std::endl;

    int frame = 0;
    while(frame < 500)
    {
        if(frame == 150)
        {
            // Trigger a hot-reload mid-flight
            spdlog::info("TRIGGERING HOT-RELOAD");
            server->triggerReload();
        }

        server->tick();
        frame++;
    }

    std::cout << "\n--- SIMULATION FINISHED ---" << std::endl;
    server->shutdown();
    return 0;
}
