#include "CoroutineManager.hpp"

#include <sqrat/sqratArray.h>
#include <sqrat/sqratFunction.h>
#include <sqrat/sqratObject.h>

#include <Usagi/Modules/Runtime/Logging/RuntimeLogger.hpp>
#include <Usagi/Modules/Scripting/Quirrel/Execution/VirtualMachine.hpp>

namespace usagi::scripting::quirrel
{
void CoroutineManager::tick_coroutines()
{
    auto v = mVirtualMachine.GetRawHandle();

    std::unordered_set<CoroutineContext> toRemove;

    for(auto && coro : mActiveCoroutines)
    {
        // 1. Check state *before* resuming
        auto state = coro.get_execution_state();

        if(state == ThreadExecutionStates::Idle)
        {
            // Coroutine finished, mark for removal
            toRemove.emplace(coro.thread_context());
            continue;
        }

        if(state == ThreadExecutionStates::Running)
        {
            mVirtualMachine.logger().error(
                " Coroutine {} still running...", coro.debug_name()
            );
            continue;
        }

        if(state == ThreadExecutionStates::Suspended)
        {
            // 2. Resume the coroutine.
            // We MUST push the return value for 'suspend()' onto the
            // COROUTINE'S stack *before* waking it up.
            // Pushing to coro.thread_context() is correct.
            // sq_pushnull(coro.thread_context());

            // 3. Call the correct resume function.
            // 'resume(true)' now maps to the new function in Coroutine.cpp
            if(SQ_FAILED(coro.resume(true)))
            {
                // Shio: Coroutine failed.
                mVirtualMachine.logger().error(
                    " Resuming coroutine {} failed.", coro.debug_name()
                );
                toRemove.emplace(coro.thread_context());
                continue;
            }

            // 4. Check state *after* resuming
            auto newState = coro.try_get_yielded_values();

            // CoroutineStates::Suspended
            if(newState.has_value())
            {
                processCommand(coro.debug_name(), newState.value());
            }
            else if(newState.error() == ThreadExecutionStates::Idle)
            {
                // Coroutine finished this tick
                toRemove.emplace(coro.thread_context());
            }
        }
    }

    // Cleanup finished coroutines
    if(!toRemove.empty())
    {
        // Shio: Cleaning up finished coroutines.
        mVirtualMachine.logger().info(
            " Cleaning up {} finished coroutines.", toRemove.size()
        );
        std::erase_if(mActiveCoroutines, [&](const Coroutine & coro) {
            if(toRemove.contains(coro.thread_context()))
            {
                // Shio: Releasing coroutine.
                mVirtualMachine.logger().info(
                    " Releasing {}", coro.debug_name()
                );
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
    // Shio: Finding coroutine factory 'GetAllEntityCoroutines'...
    mVirtualMachine.logger().info(
        " Finding coroutine factory 'GetAllEntityCoroutines'..."
    );
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
        // Shio: 'GetAllEntityCoroutines' not found in script exports.
        mVirtualMachine.logger().error(
            " 'GetAllEntityCoroutines' not found in script exports."
        );
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
        mVirtualMachine.logger().info("Registering coroutine: {}", coroId);
        // f. Store both handles for management.
        mActiveCoroutines.emplace_back(Coroutine(
            mVirtualMachine.shared_from_this(),
            mVirtualMachine.initial_stack_size(), coroId, funcObj
        ));
        // g. Start the coroutine AND CHECK FOR ERRORS
        if(SQ_FAILED(mActiveCoroutines.back().start()))
        {
            mVirtualMachine.logger().error(
                " Failed to start coroutine: {}. Removing.", coroId
            );
            // todo: get and print squirrel error
            mActiveCoroutines.pop_back();
        }
    }
    // Shio: Coroutines created.
    mVirtualMachine.logger().info(
        " {} coroutines created.", mActiveCoroutines.size()
    );
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
    // Shio: Shutting down all coroutines...
    mVirtualMachine.logger().info(
        " Shutting down all {} coroutines...", mActiveCoroutines.size()
    );
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
    const std::string_view & coroId, const std::string_view & command
)
{
    // Shio: Received command from coroutine.
    mVirtualMachine.logger().info(
        " Received command from {}: {}", coroId, command
    );
    // In a real engine, this would dispatch to game systems.
    // e.g., if (strcmp(command, "MOVE_FORWARD") == 0) {... }
}
} // namespace usagi::scripting::quirrel
