#include "Coroutine.hpp"

// clang-format off
#include <sqvm.h>
#include <sqstate.h>
#include <sqfuncproto.h>
#include <sqclosure.h>
// clang-format on

#include <sqrat/sqratObject.h>

#include <Usagi/Library/Meta/Reflection/Enums.hpp>
#include <Usagi/Modules/Runtime/Logging/RuntimeLogger.hpp>
#include <Usagi/Modules/Scripting/Quirrel/Debugger/Debugger.hpp>
#include <Usagi/Modules/Scripting/Quirrel/Execution/Exceptions.hpp>
#include <Usagi/Modules/Scripting/Quirrel/Execution/VirtualMachine.hpp>
#include <Usagi/Runtime/RAII/ScopeExitGuard.hpp>

// #include <Usagi/Runtime/RAII/ScopeExitGuard.hpp>

namespace usagi::scripting::quirrel
{
Coroutine::Coroutine(
    std::shared_ptr<VirtualMachine> root_vm,
    SQInteger                       initial_stack_size_override,
    std::string                     debug_name,
    const Sqrat::Object &           coroutine_func
)
    : RawHandleResource(
          [&] {
              // Shio: Creating coroutine
              root_vm->logger().debug(" Creating coroutine: {}", debug_name);
              auto Ret = CreateCoroutine(
                  std::move(root_vm), initial_stack_size_override,
                  std::move(debug_name), coroutine_func
              );
              return Ret;
          },
          [&](raw_handle_t & ctx) {
              ctx.root_machine->logger().debug(
                  " Destroying coroutine: {}", ctx.debug_name
              );
              // 1. Release the C++ strong reference. This allows the GC
              // to collect the thread object.
              sq_release(
                  ctx.root_machine->GetRawHandle(), &ctx.coroutine_handle
              );
              // 2. Close the thread's VM handle.
              // if(sq_getrefcount(ctx.root_machine, &ctx.coroutine_handle))
              // {
              //     `sq_close` is for root vm. DO NOT CALL IT. Otherwise, the
              //     program will CRASH.
              //     sq_close(ctx.execution_context);
              // }
          }
      )
{
}

/*
 * We can reference `base_newthread()` for how to properly implement this
 * function. Basically, a `newthread()` call from the script pushes the C
 * function itself and the script closure to the stack. So `base_newthread()`
 * simply calls `sq_newthread(root_vm)` after getting the second variable on the
 * stack, which is the reference to the closure. Then it moves the closure
 * from root vm's stack to the new thread's, preparing for the coroutine being
 * called for the first time.
 */
CoroutineStates Coroutine::CreateCoroutine(
    std::shared_ptr<VirtualMachine> root_vm,
    SQInteger                       initial_stack_size_override,
    std::string                     debug_name,
    const Sqrat::Object &           coroutine_func
)
{
    auto root_vm_handle = root_vm->get_vm();

    CoroutineStates Ret;

    Ret.root_machine = root_vm;

    const auto OpDecideStackSize = [&] {
        if(initial_stack_size_override > 0) return initial_stack_size_override;
        // The default logic in `base_newthread()`
        SQInteger SqStackSizeLogic =
            (_closure(coroutine_func.GetObject())->_function->_stacksize << 1) +
            2;
        SqStackSizeLogic = (SqStackSizeLogic < MIN_STACK_OVERHEAD + 2)
            ? MIN_STACK_OVERHEAD + 2
            : SqStackSizeLogic;
        return SqStackSizeLogic;
    };

    // --- Coroutine Creation: The Duality of Handles ---
    // We must get both the HSQUIRRELVM (for execution)
    // and an HSQOBJECT (for GC management).

    // a. Create a new, empty thread (coroutine) VM.
    // This pushes the new thread *object* onto the root_vm stack.
    const auto initial_stack_size = OpDecideStackSize();
    root_vm->logger().trace(
        "Creating coroutine `{}` with initial_stack_size={}. RootVM={}.",
        debug_name, initial_stack_size, static_cast<void *>(root_vm_handle)
    );
    Ret.execution_context =
        sq_newthread(Ret.root_machine->GetRawHandle(), initial_stack_size);

    if(!Ret.execution_context)
    {
        // Not enough memory, or other fatal error
        throw InvalidVirtualMachine("sq_newthread failed!");
    }

    Ret.debug_name = std::move(debug_name);

    // b. Get an HSQOBJECT handle to the thread object (at stack top: -1).
    // This handle is for the GC.
    // See `sq_newthread()`. It pushes the new HSQUIRRELVM onto the stack as
    // index 1.
    sq_resetobject(&Ret.coroutine_handle);
    sq_getstackobj(root_vm_handle, -1, &Ret.coroutine_handle);

    // c. Add a strong reference to the object. This tells the GC
    // that C++ is holding onto this object, preventing it from
    // being collected.
    sq_addref(root_vm_handle, &Ret.coroutine_handle);

    // d. Pop the thread object from the *main* VM's stack.
    // Our C++ coroHandle is now the only strong reference.
    // This basically does clean-up after `sq_newthread()` and resets the stack.
    sq_pop(root_vm_handle, 1);

    // Link the coroutine VM back to the root VM via foreign pointer so
    // that error handling and such can work properly.
    sq_setforeignptr(Ret.execution_context, root_vm_handle->_foreignptr);

    // --- BEGIN ENVIRONMENT FIX ---
    // A new thread has an empty root table. We must copy the
    // root table from the main VM to the new thread so it can
    // access global functions like 'native_log' and 'require'.

    // ^^ This is not true. When calling `sq_newthread(Ret.root_machine...)`,
    // The root vm is treated as a "friend vm" and the new thread will share
    // its root table, as seem in `SQVM::Init()`. So this whole "env fix"
    // is unnecessary.

    // 1. Push the root table of the main VM onto the main VM's stack.
    // sq_pushroottable(root_vm);

    // 2. Move that table from the main VM's stack to the new
    //    coroutine's VM stack.
    // sq_move(Ret.execution_context, root_vm, -1);

    // 3. Set the table (now on the coroutine's stack) as the
    //    coroutine's root table.
    // sq_setroottable(Ret.execution_context);
    // --- END ENVIRONMENT FIX ---

    // Push the root table and get it prepared for execution.
    sq_pushroottable(Ret.execution_context);

    // e. Push the script function (coroutine_func) onto the
    // *new coroutine's* stack. This is the function that
    // sq_call (in start()) will execute.
    sq_pushobject(Ret.execution_context, coroutine_func.GetObject());
    // sq_pushobject(root_vm, coroutine_func.GetObject()); incorrect. crash.

    // Now the stack is [roottable][closure] and is ready to be called

    return Ret;
}

ThreadExecutionStates Coroutine::get_execution_state() const
{
    return static_cast<ThreadExecutionStates>(sq_getvmstate(thread_context()));
}

SQRESULT Coroutine::start()
{
    root_vm()->logger().debug("Coroutine: {}", debug_name());

    // This is the "kick-off" call.
    // resumedret=false: We are not resuming from a 'suspend', so don't pass a
    // value. retval=false: We do not care about a return value from this
    // initial run.
    // todo: how the fuck to use this function???
    // sq_wakeupvm doesn't work for the first call.
    // return sq_wakeupvm(thread_context(), false, false, true, false);

    // This is the "kick-off" call.
    // We use sq_call to invoke the function for the first time.
    // The stack is: [function_to_run][this_obj(null)]
    // nparams = 1 (for the 'this' object)
    // retval = false (we don't get a return value until it's idle)
    // raiseerror = true (use the error handler)
    // sq_pushnull(thread_context()); // already done in `CreateCoroutine()`

    if(!sq_isthread(coroutine_handle()))
    {
        throw MismatchedObjectType("Coroutine is not holding a thread.");
    }

    /*
     * Ok, let's now have a look at `thread_call()` and `SQVM::StartCall()` to
     * see how to start our thread. First, you gotta make sure our object is
     * indeed a thread, this should be an invariant for our class.
     * We gotta first figure out how many arguments the coroutine expects.
     */

    // Note that if `func->_varparams == 1` the function is a variadic one.
    // `_nparameters` is for fixed number of parameters.

    /* this is for closures. calling this on a thread would crash the program.
    const auto nparams = _closure(coroutine_handle())->_function->_nparameters;
    if(nparams != 1)
    {
        sq_throwerror(
            thread_context(),
            "Currently only coroutines with no parameters are supported."
        );
    }
    */

    // f. Push a 'this' object for the call.
    // Since this is a factory-returned function, it's likely
    // already bound (via .bindenv()), but sq_call still
    // expects a 'this' on the stack. We can push its
    // environment or just null. Pushing null is safest.
    // After calling `sq_call()`, this value will be popped.
    // This makes [roottable][closure][this] on the stack.
    // sq_pushobject(thread_context(), ...);
    _internal_push_instance();

    // The coroutine VM's stack is now: [base=1][function_to_run][this_obj]
    // It is ready to be 'sq_call'ed.

    // if params=0, the call will fail saying `[wrong number of parameters
    // passed to 'UpdateCoroutine' scripts/tests/entity.nut:34 (0 passed, 1
    // required)]` (with also `sq_pushnull(Ret.execution_context);` removed)
    // return sq_call(thread_context(), 0, SQFalse, SQTrue);
    // so the one param is basically the null 'this' reference.
    // After calling, `sq_call()` will pop the args, including 'this'.
    // And if `retval==true`, the result will be pushed to the stack as the
    // first result value obtained from the script-side.
    root_vm()->logger().trace(
        "pre-call: {}", Debugger::format_stack_frame(thread_context())
    );

    const auto ExitGuard = MakeScopeExitGuard([&] {
        root_vm()->logger().trace(
            "during-call: {}", Debugger::format_stack_frame(thread_context())
        );
        // Note that at this stage the stack may contain arguments for other
        // functions calls not yet cleaned up, and local variables on the stack.
        root_vm()->logger().trace(
            "frame-values: {}", Debugger::format_stack_values(thread_context())
        );
        // Calling a coroutine should either cause it to suspend or finish.
        root_vm()->logger().trace(
            "exec-state: {}", meta::enum_to_string(get_execution_state())
        );
        // Clean up the stack to finish the call.
        // clean up     [roottable][closure][this][args...]
        // Leaving only [roottable][closure] on the stack
        sq_pop(thread_context(), sq_gettop(thread_context()) - 2);
        root_vm()->logger().trace(
            "post-call: {}", Debugger::format_stack_frame(thread_context())
        );
    });

    // Seems `sq_call` is also variadic so we just keep `[roottable][closure]`
    if(SQ_SUCCEEDED(sq_call(thread_context(), 1, SQFalse, SQTrue)))
    {
        // sq_settop(thread_context(), 1);
        // clean up [roottable][closure][this]
        // leaving [roottable][closure] on the stack
        // sq_pop(thread_context(), 1);
        return 1;
    }
    // On error, clean up the stack. Leaving only the root table.
    sq_pop(thread_context(), 1);
    // sq_settop(thread_context(), 1);
    return 0;
    // `sq_resume` only works for generators, not for coroutines which are
    // threads.
    // return sq_resume(thread_context(), SQFalse, SQTrue);
}

SQRESULT Coroutine::resume(const bool invoke_err_handler)
{
    // The roottable should already be on the stack from previous calls along
    // with the closure.
    // sq_pushroottable(thread_context());

    return _internal_resume(true, false, invoke_err_handler);
}

void Coroutine::_internal_push_instance()
{
    SQObject o;
    sq_getstackobj(thread_context(), 1, &o);
    sq_pushobject(thread_context(), o);
    // if the coroutine is bound to certain object, we must push the
    // corresponding OT_INSTANCE onto the stack, otherwise the function
    // cannot be called.
    // sq_pushnull(thread_context());
}

/*
 * For this function we will learn from `thread_wakeup()`.
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
        _internal_push_instance();
    }

    // This is the "tick" call.
    // The VM MUST be in a 'SUSPENDED' state.
    const auto state         = get_execution_state();
    SQInteger  stackTop      = 0;
    SQInteger  varsToPop     = 0;
    bool       pop_roottable = false;
    const auto exit          = MakeScopeExitGuard([&] {
        if(pop_roottable) sq_pop(thread_context(), varsToPop);
        root_vm()->logger().trace(
            "post-resume: {}", Debugger::format_stack_frame(thread_context())
        );
        // sq_settop(thread_context(), 1);
    });

    if(state == ThreadExecutionStates::Idle)
    {
        // Leave [roottable] on the stack
        pop_roottable = true;
        varsToPop     = sq_gettop(thread_context()) - 1;
        return sq_throwerror(thread_context(), "cannot wakeup a idle thread");
    }
    if(state == ThreadExecutionStates::Running)
    {
        // Leave [roottable][closure] on the stack
        pop_roottable = true;
        varsToPop     = sq_gettop(thread_context()) - 2;
        return sq_throwerror(
            thread_context(), "cannot wakeup a running thread"
        );
    }

    root_vm()->logger().trace(
        "pre-wakeup: {}", Debugger::format_stack_frame(thread_context())
    );

    // Push 'null' as the 'suspend()' return value
    // Maybe we can return other values in the future, such as remote gameplay
    // command execution results.
    // if(push_this) sq_pushnull(thread_context());

    // resumedret=true: Use the value we just pushed as the return for
    // 'suspend()'.
    // retval=false: We don't care about a return value from the
    // *function*
    if(SQ_SUCCEEDED(sq_wakeupvm(
           thread_context(), resumed_ret, SQFalse, invoke_err_handler, SQFalse
       )))
    {
        // If `resumedret=true` we have to push a value as `suspend()`'s return
        // value.
        // If `retval=true` we have to pop a return value from ... I am
        // not sure where.
        // Again, see `thread_wakeup()` for details.

        root_vm()->logger().trace(
            "wakeup-ok: {}", Debugger::format_stack_frame(thread_context())
        );
        root_vm()->logger().trace(
            "frame-values: {}", Debugger::format_stack_values(thread_context())
        );

        stackTop  = sq_gettop(thread_context());
        varsToPop = stackTop - 2; // Leave [roottable][closure] on the stack

        // coroutine finished execution. pop the closure.
        if(get_execution_state() == ThreadExecutionStates::Idle)
        {
            // Leave [roottable] on the stack
            pop_roottable = true;
            varsToPop     = stackTop - 1;
            // sq_settop(thread_context(), 1);
        }
        return 1;
    }

    root_vm()->logger().trace(
        "wakeup-fail: {}", Debugger::format_stack_frame(thread_context())
    );

    // Operation failed. Leave only [roottable] on the stack
    pop_roottable = true;
    varsToPop     = sq_gettop(thread_context()) - 1;
    // sq_settop(thread_context(), 1);
    return SQ_ERROR;
}

runtime::MaybeError<std::string, ThreadExecutionStates>
    Coroutine::try_get_yielded_values()
{
    root_vm()->logger().debug("Coroutine: {}", debug_name());

    const auto state = get_execution_state();

    // If the coroutine is not suspended, we have no value to return but an
    // execution state.
    if(state != ThreadExecutionStates::Suspended)
    {
        return { state };
    }

    HSQUIRRELVM     v        = thread_context();
    const SQInteger stackTop = sq_gettop(v);

    /*
    SQInteger    level;
    SQStackInfos si;
    sq_getinteger(v, -2, &level);
    if(SQ_SUCCEEDED(sq_stackinfos(v, level, &si)))
    {
        root_vm()->logger().debug(
            "Coroutine suspended at {}:{} in function {}.", si.source, si.line,
            si.funcname
        );
    }
    else
    {
        root_vm()->logger().debug(
            "Coroutine suspended without yielding any values."
        );
    }
    */

    /*
    // Validate that we are dealing with a coroutine.
    SQObjectPtr o = stack_get(v, 1);
    if(!_thread(o))
    {
        throw MismatchedObjectType(
            "Not having the coroutine at stackframe index 1."
        );
    }*//**/

    // --- Definitive Stack Model (based on logs) ---
    // `suspend` is a variadic C-function. The stack frame is:
    // [1]       : `this` (OT_TABLE or OT_NULL if root table not set)
    // [2]       : `suspend` (OT_NATIVECLOSURE)
    // [3]       : `prologue_null` (OT_NULL boundary)
    // [4...N-1] : `arg1, ..., argN` (The variadic arguments)
    // [N]       : `epilogue_null` (OT_NULL boundary)
    //
    // `stackTop = nArgs + 4`
    // `nArgs = stackTop - 4`

    // `suspend(..)` is a variadic function. Use `sq_gettop()` to get the number
    // of passed arguments. We can refer to `print(...)` for how to handle
    // variadic functions.
    root_vm()->logger().trace("pre-yield: {}", Debugger::format_stack_frame(v));
    // Leaving only [roottable][closure] on the stack
    if(stackTop > 3)
    {
        // todo: handle command dispatching here
        sq_pop(v, stackTop - 2);
    }
    root_vm()->logger().trace(
        "post-yield: {}", Debugger::format_stack_frame(v)
    );

    // sq_settop(thread_context(), 1);

    /*
    SQRESULT res = __sq_getcallstackinfos(thread, level);
    if(SQ_FAILED(res))
    {
        sq_settop(thread, threadtop);
        if(sq_type(thread->_lasterror) == OT_STRING)
        {
            sq_throwerror(v, _stringval(thread->_lasterror));
        }
        else
        {
            sq_throwerror(v, _SC("unknown error"));
        }
    }
    if(res > 0)
    {
        // some result
        sq_move(v, thread, -1);
        sq_settop(thread, threadtop);
        return 1;
    }
    // no result
    sq_settop(thread, threadtop);
    */


    /*
    const SQInteger nArgs = stackTop - 4;

    // This block is the logic *you* discovered and is proven correct by the
    // logs.
    if(stackTop > 3) // Your original `nArgs > 3` is `stackTop > 3`
    {
        const auto opPrintStackframe = [&] { // Log the failure
            auto        _stackTop = sq_gettop(thread_context());
            std::string stackDump =
                "Stack Dump (top=" + std::to_string(_stackTop) + "):";
            for(SQInteger i = 1; i <= _stackTop; ++i)
            {
                stackDump +=
                    " [" +
                    std::string(
                        meta::enum_to_string(sq_gettype(thread_context(), i))
                    ) +
                    "]";
            }
            root_vm()->logger().debug("Stackframe: {}", stackDump);
        };
        // opPrintStackframe(); // Debug print

        // We must pop everything *except* the `this` object at index 1.
        // `sq_wakeupvm` will *not* clean this up.
        // We pop `stackTop - 1` items.
        const auto _ = MakeScopeExitGuard([&] {
            sq_pop(v, stackTop - 1);
            // opPrintStackframe(); // Debug print
        });

        if(nArgs > 0) // Check if we *actually* have args
        {
            // At least one argument was passed.
            // The first argument is *always* at absolute index 4.
            const SQChar * command = nullptr;
            if(SQ_SUCCEEDED(sq_getstring(v, 4, &command)))
            {
                if(command)
                {
                    return std::string(command);
                }
            }
            else
            {
                // Log the failure if arg 1 exists but isn't a string.
                std::string stackDump =
                    "Stack Dump (top=" + std::to_string(stackTop) + "):";
                for(SQInteger i = 1; i <= stackTop; ++i)
                {
                    stackDump += " [" +
                        std::string(meta::enum_to_string(sq_gettype(v, i))) +
                        "]";
                }
                root_vm()->logger().warn(
                    "Coroutine {} suspended with args, but arg at index 4 was "
                    "not a string. {}",
                    debug_name(), stackDump
                );
            }
        }
        // else: nArgs == 0. This is a normal pause.
        // The ScopeExitGuard will still run and clean up the stack.
    }
    else
    {
        // stackTop <= 3. This is an unexpected/error state.
        // We should still clean it up to prevent leaks.
        if(stackTop > 1)
        {
            sq_pop(v, stackTop - 1);
        }
    }
    */

    return { state };
}
} // namespace usagi::scripting::quirrel
