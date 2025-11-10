#include "Coroutine.hpp"

// clang-format off
#include <sqvm.h>
#include <sqstate.h>
#include <sqfuncproto.h>
#include <sqclosure.h>
// clang-format on

#include <sqarray.h>
#include <sqtable.h>

#include <sqrat/sqratObject.h>

#include <Usagi/Library/Meta/Reflection/Enums.hpp>
#include <Usagi/Modules/Runtime/Logging/RuntimeLogger.hpp>
#include <Usagi/Modules/Scripting/Quirrel/Debugger/Debugger.hpp>
#include <Usagi/Modules/Scripting/Quirrel/Execution/Exceptions.hpp>
#include <Usagi/Modules/Scripting/Quirrel/Execution/VirtualMachine.hpp>
#include <Usagi/Runtime/RAII/ScopeExitGuard.hpp>

namespace usagi::scripting::quirrel
{
/**
 * \brief The constructor sets up a RAII-style resource handle that calls
 *        CreateCoroutine on construction and schedules the destruction logic
 *        (releasing the GC reference) to be called when the C++ object is
 *        destroyed.
 */
Coroutine::Coroutine(
    std::shared_ptr<VirtualMachine> root_vm,
    SQInteger                       initial_stack_size_override,
    std::string                     debug_name,
    const Sqrat::Object &           coroutine_func
)
    : RawHandleResource(
          // Shio: Construction Logic: Defer to the static CreateCoroutine
          // function.
          [&] {
              root_vm->logger().debug(" Creating coroutine: {}", debug_name);
              auto Ret = CreateCoroutine(
                  std::move(root_vm), initial_stack_size_override,
                  std::move(debug_name), coroutine_func
              );
              return Ret;
          },
          // Shio: Destruction Logic: This lambda is executed when the
          //       RawHandleResource is destroyed.
          [&](raw_handle_t & ctx) {
              ctx.root_machine->logger().debug(
                  " Destroying coroutine: {}", ctx.debug_name
              );
              // Shio: Release the C++ strong reference to the thread object.
              // This is crucial. It decrements the reference count, telling the
              // Quirrel GC that our C++ code no longer needs this object. If
              // there are no other references (e.g., from other script
              // objects), the GC is now free to collect the coroutine.
              sq_release(
                  ctx.root_machine->GetRawHandle(), &ctx.coroutine_handle
              );
              // Shio: NOTE: We DO NOT call sq_close(ctx.execution_context).
              // `sq_close` is only for the root VM. The thread VM is managed
              // by the root VM's garbage collector.
          }
      )
{
}

/**
 * \brief The static factory function that performs the complex setup for a new
 *        coroutine.
 * \details This function follows the required Quirrel C API steps to create a
 *          new thread, associate it with the root VM, and prepare it for its
 *          first execution.
 */
CoroutineStates Coroutine::CreateCoroutine(
    std::shared_ptr<VirtualMachine> root_vm,
    SQInteger                       initial_stack_size_override,
    std::string                     debug_name,
    const Sqrat::Object &           coroutine_func
)
{
    /*
     * Shio: This is Yukino's original note, which provides excellent context.
     * We can reference `base_newthread()` for how to properly implement this
     * function. Basically, a `newthread()` call from the script pushes the C
     * function itself and the script closure to the stack. So
     * `base_newthread()` simply calls `sq_newthread(root_vm)` after getting the
     * second variable on the stack, which is the reference to the closure. Then
     * it moves the closure from root vm's stack to the new thread's, preparing
     * for the coroutine being called for the first time.
     */
    auto root_vm_handle = root_vm->get_vm();

    CoroutineStates Ret;

    Ret.root_machine = root_vm;

    // Shio: Helper to determine the initial stack size for the new thread.
    // If no override is provided, it mimics Squirrel's default logic based on
    // the function's needs.
    const auto OpDecideStackSize = [&] {
        if(initial_stack_size_override > 0) return initial_stack_size_override;
        SQInteger SqStackSizeLogic =
            (_closure(coroutine_func.GetObject())->_function->_stacksize << 1) +
            2;
        SqStackSizeLogic = (SqStackSizeLogic < MIN_STACK_OVERHEAD + 2)
            ? MIN_STACK_OVERHEAD + 2
            : SqStackSizeLogic;
        return SqStackSizeLogic;
    };

    // Shio: Step 1: Create the new thread VM.
    // `sq_newthread` creates a new HSQUIRRELVM and pushes the corresponding
    // thread *object* onto the root VM's stack. We need to capture both.
    const auto initial_stack_size = OpDecideStackSize();
    root_vm->logger().trace(
        "Creating coroutine `{}` with initial_stack_size={}. RootVM={}.",
        debug_name, initial_stack_size, static_cast<void *>(root_vm_handle)
    );
    Ret.execution_context =
        sq_newthread(Ret.root_machine->GetRawHandle(), initial_stack_size);

    if(!Ret.execution_context)
    {
        throw InvalidVirtualMachine("sq_newthread failed!");
    }

    Ret.coroutine_func = coroutine_func;
    Ret.debug_name     = std::move(debug_name);

    // Shio: Step 2: Get a garbage-collectable handle (HSQOBJECT) to the thread.
    // The thread object is currently at the top of the root VM's stack.
    sq_resetobject(&Ret.coroutine_handle);
    sq_getstackobj(root_vm_handle, -1, &Ret.coroutine_handle);

    // Shio: Step 3: Add a strong reference.
    // This informs the GC that our C++ code is holding a reference, preventing
    // the thread from being prematurely collected. This is paired with
    // `sq_release` in the destructor.
    sq_addref(root_vm_handle, &Ret.coroutine_handle);

    // Shio: Step 4: Clean up the root VM's stack.
    // We now have a strong reference in `Ret.coroutine_handle`, so we can pop
    // the temporary object from the root VM's stack.
    sq_pop(root_vm_handle, 1);

    // Shio: Step 5: Link the coroutine's foreign pointer.
    // This allows the coroutine to access host-defined services, like the
    // logger, through the root VM's context.
    sq_setforeignptr(Ret.execution_context, root_vm_handle->_foreignptr);

    // Shio: Step 6: Prepare the new coroutine's stack for the first call.
    // A valid stack requires the root table at the bottom.
    sq_pushroottable(Ret.execution_context);

    // Shio: Step 7: Push the coroutine's entry point function onto its own
    // stack.
    sq_pushobject(Ret.execution_context, coroutine_func.GetObject());

    // Shio: Step 8: The coroutine function is a method of a class instance, so
    // we must also get a handle to that instance, which will serve as the
    // 'this' object for all calls.
    sq_getstackobj(Ret.execution_context, 1, &Ret.context_instance);

    // Shio: The coroutine's stack is now `[roottable] [closure]`. It is
    // correctly primed and ready for `start()` to be called.
    return Ret;
}

ThreadExecutionStates Coroutine::get_execution_state() const
{
    return static_cast<ThreadExecutionStates>(sq_getvmstate(thread_context()));
}

SQRESULT Coroutine::start()
{
    root_vm()->logger().debug("Coroutine: {}", debug_name());

    if(!sq_isthread(coroutine_handle()))
    {
        throw MismatchedObjectType("Coroutine is not holding a thread.");
    }

    mExecutionFrame = 0;

    // Shio: Prepare the stack for the initial call. The stack currently
    // contains
    // `[roottable] [closure]`. `sq_call` for a method requires a 'this' object.
    _internal_push_instance();
    // Shio: The stack is now `[roottable] [closure] [this_instance]`.

    root_vm()->logger().trace(
        "pre-call: {}", Debugger::format_stack_frame(thread_context())
    );

    const auto ExitGuard = MakeScopeExitGuard([&] {
        // Shio: This block executes after sq_call returns, either successfully,
        // with an error, or due to suspension. It's for logging the outcome.
        root_vm()->logger().trace(
            "during-call: {}", Debugger::format_stack_frame(thread_context())
        );

        // Note that at this stage the stack may contain arguments for other
        // functions calls not yet cleaned up, and local variables on the stack.
        root_vm()->logger().trace(
            "frame-values: {}", Debugger::format_stack_values(thread_context())
        );
        root_vm()->logger().trace(
            "post-call-state: {}", meta::enum_to_string(get_execution_state())
        );
        root_vm()->logger().trace(
            "post-call: {}", Debugger::format_stack_frame(thread_context())
        );
        ++mExecutionFrame;
    });

    // Shio: --- The Critical First Call & The Messy Stack Problem ---
    //
    // 1. WHY `sq_call`?
    //    According to the Quirrel API, a new thread in the 'idle' state MUST be
    //    started with `sq_call`. `sq_wakeupvm` will fail if called on an idle
    //    thread.
    //
    // 2. THE PROBLEM:
    //    The official documentation states that if `sq_call` is suspended (by
    //    `suspend()` in the script), it does NOT clean up the stack. It leaves
    //    the entire call frame (the closure, 'this' object, arguments, and all
    //    intermediate values from within the function) frozen on the stack.
    //    This is the "messy stack" you have observed.
    //
    // 3. Yukino's Finding (very important):
    /*
     * If your coroutine contains local or intermediate variables, it is
     * important to note that `sq_call()` will **NOT** clean the local or
     * temporary variables on the stack. The arguments of your `suspend(...)`
     * call will be mixed with them. A workaround is to **ALWAYS** put a
     * `suspend(...)` call at the first line of the coroutine function and cause
     * the function to yield immediately. This way, though the local and
     * temporary variables will still be on the stack, they will all be `NULL`,
     * so if you scan from `sq_get...()` from `idx=4` upwards, you will only get
     * the arguments passed from `suspend(...)`. **DO NOT** reset the stack
     * during the first call to the coroutine as doing so will corrupt the
     * stack, preventing local and intermediate variables from being initialized
     * later. During exactly the next frame, push the whole stack of
     * `[roottable][closure][this][args...]` and call `sq_wakeupvm()` and this
     * time the vm will finish what left to do during the first call, and it's
     * now safe to reset the stack to `[roottable][closure]` again.
     */
    //
    // 4. OUR SOLUTION:
    //    We embrace this behavior. We use `sq_call` for the first run and do
    //    *not* attempt to clean the stack on suspend. The "messy" stack is
    //    internal VM state required for the next `sq_wakeupvm` to work. Our
    //    only job is to retrieve any yielded values from the top of this stack.
    //
    // 5. `nparams = 1`:
    //    Yukino's finding: if params=0, the call will fail saying `[wrong
    //    number of parameters passed to 'UpdateCoroutine' ... (0 passed, 1
    //    required)]`. This proves that `sq_call` requires a parameter for the
    //    'this' object, even if the function takes no script arguments.
    //    Therefore, we use `nparams = 1`.
    if(SQ_SUCCEEDED(sq_call(thread_context(), 1, SQFalse, SQTrue)))
    {
        // Shio: Success here means one of two things:
        // 1. The coroutine ran to completion without suspending.
        // 2. The coroutine ran until it hit a `suspend()` call.
        // In both cases, the call itself was successful.
        return 1;
    }

    // Shio: The call failed with a compile or runtime error.
    return 0;

    // Shio: --- INCORRECT USAGE EXAMPLE ---
    // return sq_wakeupvm(thread_context(), false, false, true, false);
    // Why it's wrong: `sq_wakeupvm` cannot start a coroutine that is in the
    // 'idle' state. It will fail. It is only for resuming a 'suspended' one.

    // Shio: --- INCORRECT USAGE EXAMPLE ---
    // if(SQ_SUCCEEDED(sq_call(thread_context(), 1, SQFalse, SQTrue))) {
    //     sq_settop(thread_context(), 2); // Attempt to clean the stack
    // }
    // Why it's wrong: If the `sq_call` was suspended, the stack contains not
    // just our initial setup, but the entire frozen call frame required by the
    // VM for the next `sq_wakeupvm`. Manually altering the stack by popping or
    // setting the top will corrupt this frozen state, and the next `resume()`
    // will fail or crash. The "messy" stack after a suspended `sq_call` is
    // internal state and must not be touched.
}

SQRESULT Coroutine::resume(const bool invoke_err_handler)
{
    // Shio: This overload resumes the coroutine without passing any value back.
    // In the script, the `suspend()` call will return `null`.
    // `resumed_ret = false` tells `sq_wakeupvm` that we are not providing a
    // return value.
    return _internal_resume(true, false, invoke_err_handler);
}

void Coroutine::_internal_push_instance()
{
    // Shio: Pushes the 'this' object that the coroutine function is bound to.
    // This is required for any method call, including the initial `sq_call`
    // and subsequent `resume` calls if they were to need it (though `wakeup`
    // doesn't operate like `call`).
    sq_pushobject(thread_context(), context_instance());
}

/**
 * \brief The internal, core implementation for resuming a coroutine.
 * \details This function wraps `sq_wakeupvm`, which is the correct API for
 *          resuming a thread in the 'suspended' state.
 */
SQRESULT Coroutine::_internal_resume(
    const bool push_this, const bool resumed_ret, const bool invoke_err_handler
)
{
    root_vm()->logger().debug("Coroutine: {}", debug_name());

    root_vm()->logger().trace(
        "pre-resume: {}", Debugger::format_stack_frame(thread_context())
    );

    if(push_this)
    {
        // Shio: This was part of a previous debugging attempt. As we
        // discovered, modifying the stack before `sq_wakeupvm` (other than
        // pushing the single return value for `suspend`) leads to corruption.
        // The correct 'this' instance is already part of the frozen stack
        // state. _internal_push_instance();
    }

    // Shio: This is a "tick" call. The VM MUST be in a 'suspended' state.
    const auto state = get_execution_state();

    const auto exit = MakeScopeExitGuard([&] {
        root_vm()->logger().trace(
            "post-resume: {}", Debugger::format_stack_frame(thread_context())
        );
        ++mExecutionFrame;
    });

    // Shio: It is an error to try to wake up a coroutine that is not suspended.
    if(state == ThreadExecutionStates::Idle)
    {
        return sq_throwerror(thread_context(), "cannot wakeup a idle thread");
    }
    if(state == ThreadExecutionStates::Running)
    {
        return sq_throwerror(
            thread_context(), "cannot wakeup a running thread"
        );
    }

    root_vm()->logger().trace(
        "pre-wakeup: {}", Debugger::format_stack_frame(thread_context())
    );

    // Shio: --- The Correct Resumption Call ---
    // `sq_wakeupvm` is the correct API for all calls after the first `start()`.
    //
    // Parameters for sq_wakeupvm(vm, resumed_ret, retval, raiseerr,
    // out_running):
    // - resumed_ret: A boolean indicating if we are pushing a return value for
    //   the `suspend()` call. If true, `sq_wakeupvm` expects a value to have
    //   been pushed onto the stack just before this call.
    // - retval: SQFalse. Similar to `start()`, we are not expecting a final
    //   return value from the entire function, as it will just suspend again.
    //   Yielded values are handled separately.
    // - raiseerr: True, to use the registered error handler.
    // - out_running: Not used here.
    if(SQ_SUCCEEDED(sq_wakeupvm(
           thread_context(), resumed_ret, SQFalse, invoke_err_handler, SQFalse
       )))
    {
        root_vm()->logger().trace(
            "wakeup-ok: {}", Debugger::format_stack_frame(thread_context())
        );
        root_vm()->logger().trace(
            "frame-values: {}", Debugger::format_stack_values(thread_context())
        );

        // Shio: If the coroutine finished, its state will now be 'Idle'.
        if(get_execution_state() == ThreadExecutionStates::Idle)
        {
            // Shio: The coroutine is done. The stack contains the roottable and
            // the final return value of the function. We can clean it up.
            sq_settop(thread_context(), 1);
        }
        return 1;
    }

    root_vm()->logger().trace(
        "wakeup-fail: {}", Debugger::format_stack_frame(thread_context())
    );

    // Shio: The wakeup failed. Clean the stack to a known good state (just the
    // roottable) to prevent further errors.
    sq_settop(thread_context(), 1);
    return SQ_ERROR;
}

runtime::MaybeError<std::string, ThreadExecutionStates>
    Coroutine::try_get_yielded_values()
{
    root_vm()->logger().debug("Coroutine: {}", debug_name());

    const auto state = get_execution_state();

    // Shio: We can only get yielded values if the coroutine is suspended.
    if(state != ThreadExecutionStates::Suspended)
    {
        return { state };
    }

    HSQUIRRELVM     v        = thread_context();
    const SQInteger stackTop = sq_gettop(v);

    // Shio: --- The Convention-Based `suspend` Value Retrieval ---
    //
    // ** THE PROBLEM: **
    // The `suspend(...)` function in script is variadic. The C++ host has no
    // direct way of knowing if the script called `suspend("cmd")` (1 value) or
    // `suspend("cmd", 1, 2)` (3 values). This makes reliably reading the values
    // and cleaning the stack impossible without a strict convention.
    //
    // ** THE SOLUTION: **
    // The script side MUST agree to always pass a SINGLE object (a table or
    // array) that contains all the data it wants to yield.
    //
    // Correct Script Usage:
    // suspend({ cmd = "MY_COMMAND", target_id = 123 });
    //
    // Incorrect Script Usage (Ambiguous for C++):
    // suspend("MY_COMMAND", 123);
    //
    // With this convention, the C++ host knows it only ever has to deal with
    // one object on the stack.

    // Shio: After a suspend, the yielded value(s) are at the top of the stack.
    // Because of our convention, we expect exactly ONE object (the table).
    // The stack layout is `[...internal state...] [yielded_table]`.
    // We can get the top, process it, and pop it, leaving the internal state
    // pristine for the next `sq_wakeupvm` call.

    // Shio: Let's check if there is any yielded value.
    // The base stack contains `[roottable] [closure] [this]`. So a stack top
    // greater than 3 means there are yielded values.
    // NOTE: This logic is fragile and depends on the exact state left by
    // `sq_call` or `sq_wakeupvm`. A more robust method might be needed if the
    // base stack size changes.
    /*
    if(stackTop > 3)
    {
        // Shio: For now, we assume the first yielded value is a command string.
        // A more robust implementation would get the table from the top of the
        // stack and parse it.
        const SQChar * command = nullptr;
        // Shio: The yielded value is at the top of the stack.
        if(SQ_SUCCEEDED(sq_getstring(v, -1, &command)))
        {
            if(command)
            {
                std::string result = command;
                // Shio: IMPORTANT: Clean up the yielded value(s) from the stack
                // after processing. Here we assume one value was yielded.
                sq_pop(v, 1);
                return result;
            }
        }
        // Shio: If we are here, a value was yielded but it wasn't a string, or
        // we failed to get it. We should still clean it up.
        sq_pop(v, 1);
    }
    */
    root_vm()->logger().trace(
        "frame-values: {}", Debugger::format_stack_values(thread_context())
    );

    // Shio: No values were yielded, or they were not in the expected format.
    return { state };
}
} // namespace usagi::scripting::quirrel
