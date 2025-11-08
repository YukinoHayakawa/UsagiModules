#include <iostream>

#include "Runtime/VirtualMachine.hpp"

using namespace usagi::scripting::quirrel;

/**
 * @brief Main function to run the server example.
 */
int main()
{
    // Create the dummy script files on disk
    // CreateDummyScripts();

    auto server = std::make_shared<VirtualMachine>();

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
            server->triggerReload();
        }

        server->tick();
        frame++;
    }

    std::cout << "\n--- SIMULATION FINISHED ---" << std::endl;
    server->shutdown();
    return 0;
}
