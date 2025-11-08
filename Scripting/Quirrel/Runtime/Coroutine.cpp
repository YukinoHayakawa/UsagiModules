#include "Coroutine.hpp"

#include <spdlog/spdlog.h>
#include <sqrat/sqratObject.h>

#include <Usagi/Runtime/Exceptions/Exceptions.hpp>
#include <Usagi/Runtime/RAII/ScopeExitGuard.hpp>

namespace usagi::scripting::quirrel
{
Coroutine::Coroutine(
    const HSQUIRRELVM     root_vm,
    const std::uint64_t   initial_stack_size,
    std::string           debug_name,
    const Sqrat::Object & coroutine_func)
    : RawHandleResource(
          [&] {
              // Shio: Creating coroutine
              spdlog::debug(" Creating coroutine: {}", debug_name);
              return CreateCoroutine(
                  root_vm, initial_stack_size, coroutine_func);
          },
          [=](RawHandleT & ctx) {
              spdlog::debug(" Destroying coroutine: {}", debug_name);
              // 1. Release the C++ strong reference. This allows the GC
              // to collect the thread object.
              sq_release(ctx.root_machine, &ctx.coroutine_handle);
              // 2. Close the thread's VM handle.
              // if(sq_getrefcount(ctx.root_machine, &ctx.coroutine_handle))
              // {
              //     `sq_close` is for root vm. DO NOT CALL IT.
              //     sq_close(ctx.execution_context);
              // }
          })
    , mDebugName(std::move(debug_name))
{
}

CoroutineStates Coroutine::CreateCoroutine(
    HSQUIRRELVM           root_vm,
    std::uint64_t         initial_stack_size,
    const Sqrat::Object & coroutine_func)
{
    if(!root_vm)
    {
        throw runtime::RuntimeError("Root HSQUIRRELVM is nullptr!");
    }

    CoroutineStates Ret;

    Ret.root_machine = root_vm;

    // --- Coroutine Creation: The Duality of Handles ---
    // We must get both the HSQUIRRELVM (for execution)
    // and an HSQOBJECT (for GC management).

    // a. Create a new, empty thread (coroutine) VM.
    // This pushes the new thread *object* onto the root_vm stack.
    Ret.execution_context = sq_newthread(root_vm, initial_stack_size);
    if(!Ret.execution_context)
    {
        throw runtime::RuntimeError("sq_newthread failed!");
    }

    // b. Get an HSQOBJECT handle to the thread object (at stack top: -1).
    // This handle is for the GC.
    sq_resetobject(&Ret.coroutine_handle);
    sq_getstackobj(root_vm, -1, &Ret.coroutine_handle);

    // c. Add a strong reference to the object. This tells the GC
    // that C++ is holding onto this object, preventing it from
    // being collected.
    sq_addref(root_vm, &Ret.coroutine_handle);

    // d. Pop the thread object from the *main* VM's stack.
    // Our C++ coroHandle is now the only strong reference.
    sq_pop(root_vm, 1);

    // e. *** THE FIX ***
    // Push the script function (coroutine_func) onto the
    // *new coroutine's* stack. This is the function that
    // sq_wakeupvm (in start()) will execute.
    sq_pushobject(Ret.execution_context, coroutine_func.GetObject());

    // The coroutine VM's stack is now: [base=1][function_to_run]
    // It is ready to be started.

    return Ret;
}

CoroutineExecutionStates Coroutine::get_state() const
{
    return static_cast<CoroutineExecutionStates>(
        sq_getvmstate(thread_context()));
}

SQRESULT Coroutine::start()
{
    // This is the "kick-off" call.
    // resumedret=false: We are not resuming from a 'suspend', so don't pass a value.
    // retval=false: We do not care about a return value from this initial run.
    // todo: how the fuck to use this function???
    return sq_wakeupvm(thread_context(), false, false, true, false);
}

SQRESULT Coroutine::resume(bool invoke_err_handler)
{
    // Push null as the 'suspend()' return value
    sq_pushnull(thread_context());
    // resumedret=true: Use the value we just pushed as the return for 'suspend()'.
    // retval=false: We don't care about a return value from the *function*
    // (only from 'yield').
    return sq_wakeupvm(
        thread_context(), true, false, invoke_err_handler, false);
}

runtime::MaybeError<std::string, CoroutineExecutionStates>
    Coroutine::try_get_yielded_command()
{
    const auto state = get_state();

    if(state != CoroutineExecutionStates::Suspended)
    {
        return { state };
    }

    // 4. Coroutine yielded. Check stack for a command.
    SQInteger stackTop = sq_gettop(thread_context());

    // stackTop=1 is 'this'. >1 means a value was yielded.
    if(stackTop > 1)
    {
        // 5. CRITICAL: Pop all yielded values to clean the
        // stack
        const auto _ =
            MakeScopeExitGuard([&] { sq_pop(thread_context(), stackTop - 1); });

        const SQChar * command = nullptr;
        // Check if the yielded value (at -1) is a string
        if(SQ_SUCCEEDED(sq_getstring(thread_context(), -1, &command)))
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
