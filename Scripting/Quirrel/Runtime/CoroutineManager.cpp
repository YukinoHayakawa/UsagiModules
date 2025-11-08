#include "CoroutineManager.hpp"

#include <iostream>

#include <spdlog/spdlog.h>
#include <sqrat/sqratArray.h>
#include <sqrat/sqratFunction.h>
#include <sqrat/sqratObject.h>

#include <Usagi/Runtime/Exceptions/Exceptions.hpp>

#include "VirtualMachine.hpp"

namespace usagi::scripting::quirrel
{
void CoroutineManager::tick_coroutines()
{
    auto v = mVirtualMachine.GetRawHandle();

    std::unordered_set<CoroutineContext> toRemove;

    for(auto && coro : mActiveCoroutines)
    {
        // 1. Check state *before* resuming
        auto state = coro.get_state();

        if(state == CoroutineStates::Idle)
        {
            // Coroutine finished, mark for removal
            toRemove.emplace(coro.thread_context());
            continue;
        }

        if(state == CoroutineStates::Suspended)
        {
            // 2. Resume the coroutine. We push a 'null' which will be the
            // return value of the 'yield' statement in the script.
            sq_pushnull(v); // Using main VM 'v' to push onto coroutine
                            // 'coro.vm' is OK
            if(SQ_FAILED(coro.resume(false)))
            {
                std::cerr << " Coroutine " << coro.debug_name() << " failed."
                          << std::endl;
                toRemove.emplace(coro.thread_context());
                continue;
            }

            // 3. Check state *after* resuming
            auto newState = coro.try_get_yielded_command();

            // CoroutineStates::Suspended
            if(newState.has_value())
            {
                processCommand(coro.debug_name(), newState.value());
            }
            else if(newState.error() == CoroutineStates::Idle)
            {
                // Coroutine finished this tick
                toRemove.emplace(coro.thread_context());
            }
        }
    }

    // Cleanup finished coroutines
    if(!toRemove.empty())
    {
        std::cout << " Cleaning up " << toRemove.size()
                  << " finished coroutines." << std::endl;
        std::erase_if(mActiveCoroutines, [&](const Coroutine & coro) {
            if(toRemove.contains(coro.thread_context()))
            {
                std::cout << " Releasing " << coro.debug_name() << std::endl;
                // sq_release(v, (HSQOBJECT *)&coro.handle);
                // sq_close(coro.vm);
                return true;
            }
            return false;
        });
    }
}

void CoroutineManager::_findAndCreateCoroutines(Sqrat::Object & exports)
{
    std::cout << " Finding coroutine factory 'GetAllEntityCoroutines'..."
              << std::endl;
    // try
    // {
    auto v = mVirtualMachine.GetRawHandle();

    // 1. Find the factory function in the script's exports
    // todo: customize coroutine names
    // Sqrat::Function getCoroutines =
    // exports.GetSlot<Sqrat::Function>("GetAllEntityCoroutines");
    // !!! this is the correct syntax !!!
    Sqrat::Function getCoroutines(exports, "GetAllEntityCoroutines");

    if(getCoroutines.IsNull())
    {
        std::cerr << " 'GetAllEntityCoroutines' not found in script exports."
                  << std::endl;
        return;
    }

    // 2. Call the factory function to get an array of functions
    // Sqrat::Array coroFuncs = getCoroutines.Evaluate<Sqrat::Array>();
    Sqrat::Array coroFuncs;
    // !!! This is how to call functions !!!
    // !!! See sqmodules.cpp for more examples !!!
    if(!getCoroutines.Evaluate(coroFuncs))
    {
        sq_throwerror(v, "Failed to call `GetAllEntityCoroutines()`");
    }

    // 3. Create a coroutine for each function in the array
    for(SQInteger i = 0; i < coroFuncs.Length(); ++i)
    {
        Sqrat::Object funcObj = coroFuncs.GetValue<Sqrat::Object>(i);
        std::string   coroId  = "Coroutine_" + std::to_string(i);
        spdlog::info("Registering coroutine: {}", coroId);
        // f. Store both handles for management.
        mActiveCoroutines.emplace_back(
            mVirtualMachine.CreateBindNewObject<Coroutine>(
                mVirtualMachine.initial_stack_size(), coroId, funcObj));
    }
    std::cout << " " << mActiveCoroutines.size() << " coroutines created."
              << std::endl;
    // }
    // todo there is no such a thing called Sqrat::Exception
    // catch(Sqrat::Exception & e)
    // {
    //     std::cerr << " Failed to create coroutines: " << e.Message().c_str()
    //               << std::endl;
    // }
}

void CoroutineManager::_shutdownAllCoroutines()
{
    std::cout << " Shutting down all " << mActiveCoroutines.size()
              << " coroutines..." << std::endl;
    /*for(auto & coro : mActiveCoroutines)
    {
        // 1. Release the C++ strong reference. This allows the GC
        // to collect the thread object.
        sq_release(v, &coro.handle);

        // 2. Close the thread's VM handle.
        sq_close(coro.vm);
    }*/
    mActiveCoroutines.clear();
}

void CoroutineManager::processCommand(
    const std::string_view & coroId, const std::string_view & command)
{
    std::cout << " Received command from " << coroId << ": " << command
              << std::endl;
    // In a real engine, this would dispatch to game systems.
    // e.g., if (strcmp(command, "MOVE_FORWARD") == 0) {... }
}
} // namespace usagi::scripting::quirrel
