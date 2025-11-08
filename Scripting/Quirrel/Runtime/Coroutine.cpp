#include "Coroutine.hpp"

#include <spdlog/spdlog.h>
#include <sqrat/sqratObject.h>

#include <Usagi/Runtime/RAII/ScopeExitGuard.hpp>

namespace usagi::scripting::quirrel
{
Coroutine::Coroutine(
    const HSQUIRRELVM     root_vm,
    const std::uint64_t   initial_stack_size,
    std::string           debug_name,
    const Sqrat::Object & coroutine_func)
    : mRootMachine(root_vm)
    , mDebugName(std::move(debug_name))
{
    // Shio: Creating coroutine
    spdlog::info(" Creating coroutine: {}", debug_name);

    // --- Coroutine Creation: The Duality of Handles ---
    // We must get both the HSQUIRRELVM (for execution)
    // and an HSQOBJECT (for GC management).

    // a. Push the Quirrel function object onto the *main* VM's
    // stack
    sq_pushobject(root_vm, coroutine_func.GetObject());

    // b. Create a new thread (coroutine) from the function on the
    // stack. This pops the function and pushes the new thread
    // object.
    mThreadExecution =
        sq_newthread(root_vm, initial_stack_size); // 1024 stack size

    // c. Get an HSQOBJECT handle to the thread object (which is at
    // stack top: -1)
    sq_resetobject(&mCoroutineHandle);
    sq_getstackobj(root_vm, -1, &mCoroutineHandle);

    // d. Add a strong reference to the object. This tells the GC
    // that C++ is holding onto this object, preventing it from
    // being collected.
    sq_addref(root_vm, &mCoroutineHandle);

    // e. Pop the thread object from the *main* VM's stack.
    // Our C++ coroHandle is now the only strong reference.
    sq_pop(root_vm, 1);
}

Coroutine::~Coroutine()
{
    // 1. Release the C++ strong reference. This allows the GC
    // to collect the thread object.
    sq_release(mRootMachine, &mCoroutineHandle);

    // 2. Close the thread's VM handle.
    sq_close(mThreadExecution);
}

CoroutineStates Coroutine::get_state() const
{
    return static_cast<CoroutineStates>(sq_getvmstate(mThreadExecution));
}

SQRESULT Coroutine::resume(bool ret_value, bool invoke_err_handler)
{
    return sq_resume(mThreadExecution, ret_value, invoke_err_handler);
}

runtime::MaybeError<std::string, CoroutineStates>
Coroutine::try_get_yielded_command()
{
    const auto state = get_state();

    if(state != CoroutineStates::Suspended)
    {
        return { state };
    }

    // 4. Coroutine yielded. Check stack for a command.
    SQInteger stackTop = sq_gettop(mThreadExecution);

    // stackTop=1 is 'this'. >1 means a value was yielded.
    if(stackTop > 1)
    {
        // 5. CRITICAL: Pop all yielded values to clean the
        // stack
        const auto _ =
            MakeScopeExitGuard([&] { sq_pop(mThreadExecution, stackTop - 1); });

        const SQChar * command = nullptr;
        // Check if the yielded value (at -1) is a string
        if(SQ_SUCCEEDED(sq_getstring(mThreadExecution, -1, &command)))
        {
            if(command)
            {
                return std::string(command);
                // processCommand(coro.id, command);
            }
        }
        // sq_pop(coro.vm, stackTop - 1);
    }

    return { state };
}
} // namespace usagi::scripting::quirrel
