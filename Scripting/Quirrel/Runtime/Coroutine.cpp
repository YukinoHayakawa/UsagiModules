#include "Coroutine.hpp"

#include <spdlog/spdlog.h>
#include <sqrat/sqratObject.h>

#include <Usagi/Library/Reflection/StaticReflection.hpp>
#include <Usagi/Runtime/Exceptions/Exceptions.hpp>
#include <Usagi/Runtime/RAII/ScopeExitGuard.hpp>

namespace usagi::scripting::quirrel
{
// Helper function to convert SQObjectType to string
inline std::string_view sq_typetostring(SQObjectType type)
{
    switch(type)
    {
        case OT_NULL         : return "OT_NULL";
        case OT_INTEGER      : return "OT_INTEGER";
        case OT_FLOAT        : return "OT_FLOAT";
        case OT_BOOL         : return "OT_BOOL";
        case OT_STRING       : return "OT_STRING";
        case OT_TABLE        : return "OT_TABLE";
        case OT_ARRAY        : return "OT_ARRAY";
        case OT_USERDATA     : return "OT_USERDATA";
        case OT_CLOSURE      : return "OT_CLOSURE";
        case OT_NATIVECLOSURE: return "OT_NATIVECLOSURE";
        case OT_GENERATOR    : return "OT_GENERATOR";
        case OT_USERPOINTER  : return "OT_USERPOINTER";
        case OT_THREAD       : return "OT_THREAD";
        case OT_CLASS        : return "OT_CLASS";
        case OT_INSTANCE     : return "OT_INSTANCE";
        case OT_WEAKREF      : return "OT_WEAKREF";
        default              : return "OT_UNKNOWN";
    }
}

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
        // Not enough memory, or other fatal error
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

    // --- BEGIN ENVIRONMENT FIX ---
    // A new thread has an empty root table. We must copy the
    // root table from the main VM to the new thread so it can
    // access global functions like 'native_log' and 'require'.

    // 1. Push the root table of the main VM onto the main VM's stack.
    sq_pushroottable(root_vm);

    // 2. Move that table from the main VM's stack to the new
    //    coroutine's VM stack.
    sq_move(Ret.execution_context, root_vm, -1);

    // 3. Set the table (now on the coroutine's stack) as the
    //    coroutine's root table.
    sq_setroottable(Ret.execution_context);
    // --- END ENVIRONMENT FIX ---

    // e. Push the script function (coroutine_func) onto the
    // *new coroutine's* stack. This is the function that
    // sq_call (in start()) will execute.
    sq_pushobject(Ret.execution_context, coroutine_func.GetObject());
    // sq_pushobject(root_vm, coroutine_func.GetObject()); incorrect. crash.

    // f. Push a 'this' object for the call.
    // Since this is a factory-returned function, it's likely
    // already bound (via .bindenv()), but sq_call still
    // expects a 'this' on the stack. We can push its
    // environment or just null. Pushing null is safest.
    // sq_pushobject(Ret.execution_context, coroutine_func.GetObject());
    sq_pushnull(Ret.execution_context);

    // The coroutine VM's stack is now: [base=1][function_to_run][this_obj]
    // It is ready to be 'sq_call'ed.

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
    // sq_pushnull(thread_context());

    // if params=0, the call will fail saying `[wrong number of parameters
    // passed to 'UpdateCoroutine' scripts/tests/entity.nut:34 (0 passed, 1
    // required)]` (with also `sq_pushnull(Ret.execution_context);` removed)
    // return sq_call(thread_context(), 0, SQFalse, SQTrue);
    return sq_call(thread_context(), 1, SQFalse, SQTrue);
    // return sq_resume(thread_context(), SQFalse, SQTrue);
}

SQRESULT Coroutine::resume(bool invoke_err_handler)
{
    // This is the "tick" call.
    // The VM MUST be in a 'SUSPENDED' state.

    // Push 'null' as the 'suspend()' return value
    sq_pushnull(thread_context());

    // resumedret=true: Use the value we just pushed as the return for
    // 'suspend()'. retval=false: We don't care about a return value from the
    // *function*
    return sq_wakeupvm(
        thread_context(), SQTrue, SQFalse, invoke_err_handler, SQFalse);
}

runtime::MaybeError<std::string, CoroutineExecutionStates>
    Coroutine::try_get_yielded_command()
{
    const auto state = get_state();

    /*
    if(state != CoroutineExecutionStates::Suspended)
    {
        return { state };
    }

    // Stack analysis for _vm_suspend (varargs function)
    // suspend()        -> sq_gettop() == 1. Stack: [this]
    // suspend("cmd")   -> sq_gettop() == 2. Stack: [this, "cmd"]
    // suspend("cmd", 1) -> sq_gettop() == 3. Stack: [this, "cmd", 1]

    SQInteger stackTop = sq_gettop(thread_context());

    // We only care about the case where a command was passed.
    if(stackTop > 1) // 1 == just [this]
    {
        // A value was passed. Pop *all* arguments AND 'this'.
        // The *entire* call frame must be popped.
        const auto _ =
            MakeScopeExitGuard([&] { sq_pop(thread_context(), stackTop - 1); });

        // --- BEGIN DEBUGGING ---
        // Get the type of the object at the top of the stack
        SQObjectType type = sq_gettype(thread_context(), -1);
        // --- END DEBUGGING ---

        // Check if the TOP argument (at -1) is a string
        const SQChar * command = nullptr;
        if(SQ_SUCCEEDED(sq_getstring(thread_context(), -1, &command)))
        {
            if(command)
            {
                return std::string(command);
            }
        }
        else
        {
            // Log the type we found
            spdlog::warn(
                "Coroutine {} suspended with a value, but it was not a string. "
                "Type was: {}. stackTop={}.",
                mDebugName, sq_typetostring(type), stackTop);
        }
    }
    else
    {
        // stackTop == 1. suspend() was called with no args.
        // We must *still* pop the 'this' object.
        // sq_pop(thread_context(), 1);
    }
    */

    /*
    if(state != CoroutineExecutionStates::Suspended)
    {
        return { state };
    }

    SQInteger stackTop = sq_gettop(thread_context());

    // Stack model: [this_obj, arg1, ..., argN, suspend_closure]
    // stackTop = 2 for 0 args
    // stackTop = 3 for 1 arg
    // stackTop = N+2 for N args

    // We are responsible for popping *only the arguments*.
    // The VM will pop 'this' and 'closure' upon resumption.
    const SQInteger nArgs = stackTop;    // - 2;

    const auto opPrintStackframe = [&] { // Log the failure
        auto        _stackTop = sq_gettop(thread_context());
        std::string stackDump =
            "Stack Dump (top=" + std::to_string(_stackTop) + "):";
        for(SQInteger i = 1; i <= _stackTop; ++i)
        {
            stackDump += " [" +
                std::string(sq_typetostring(sq_gettype(thread_context(), i))) +
                "]";
        }
        spdlog::warn("Stackframe: {}", stackDump);
    };
    if(nArgs > 3) // At least one argument was passed
    {
        opPrintStackframe();
        // We must pop the args *after* we read them, but *before*
        // resume() pushes the return value.
        const auto _ = MakeScopeExitGuard([&] {
            sq_pop(thread_context(), nArgs - 1);
            opPrintStackframe();
        });
        // The last argument is at index -2 (before the closure at -1)
        const SQChar * command = nullptr;
        if(SQ_SUCCEEDED(sq_getstring(thread_context(), 4, &command)))
        {
            if(command)
            {
                return std::string(command);
            }
        }
        else
        {
            spdlog::warn(
                "Coroutine {} suspended with args, but -2 was not a string.",
                mDebugName);
        }
    }
    // else: nArgs == 0. suspend() was called with no args.
    // This is a normal pause, not a command. Do nothing.
    */

    /*
    if(state != CoroutineExecutionStates::Suspended)
    {
        return { state };
    }

    HSQUIRRELVM v        = thread_context();
    SQInteger   stackTop = sq_gettop(v);

    // --- Definitive Stack Model (based on logs) ---
    // `suspend` is a variadic C-function. The stack frame is:
    // [1]     : `this` (OT_TABLE or OT_NULL if root table not set)
    // [2]     : `suspend` (OT_NATIVECLOSURE)
    // [3]     : `prologue_null` (OT_NULL boundary)
    // [4...N-1] : `arg1, ..., argN` (The variadic arguments)
    // [N]     : `epilogue_null` (OT_NULL boundary)
    //
    // `stackTop = nArgs + 4`
    // `nArgs = stackTop - 4`

    const SQInteger nArgs = stackTop - 4;

    // `stackTop == 4` means `nArgs == 0`. This is a normal pause.
    if(nArgs > 0)
    {
        // We must pop everything *except* the `this` object at index 1.
        // `sq_wakeupvm` will *not* clean this up.
        // We pop `stackTop - 1` items.
        const auto _ = MakeScopeExitGuard([&] { sq_pop(v, stackTop - 1); });

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
                stackDump +=
                    " [" + std::string(sq_typetostring(sq_gettype(v, i))) + "]";
            }
            spdlog::warn(
                "Coroutine {} suspended with args, but arg at index 4 was not "
                "a string. {}",
                mDebugName, stackDump);
        }
    }
    // else: nArgs == 0. This is a normal pause.
    */


    if(state != CoroutineExecutionStates::Suspended)
    {
        return { state };
    }

    HSQUIRRELVM v        = thread_context();
    SQInteger   stackTop = sq_gettop(v);

    // --- Definitive Stack Model (based on logs) ---
    // `suspend` is a variadic C-function. The stack frame is:
    // [1]     : `this` (OT_TABLE or OT_NULL if root table not set)
    // [2]     : `suspend` (OT_NATIVECLOSURE)
    // [3]     : `prologue_null` (OT_NULL boundary)
    // [4...N-1] : `arg1, ..., argN` (The variadic arguments)
    // [N]     : `epilogue_null` (OT_NULL boundary)
    //
    // `stackTop = nArgs + 4`
    // `nArgs = stackTop - 4`

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
                stackDump += " [" +
                    std::string(sq_typetostring(
                        sq_gettype(thread_context(), i))) +
                    "]";
            }
            spdlog::warn("Stackframe: {}", stackDump);
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
                        std::string(sq_typetostring(sq_gettype(v, i))) + "]";
                }
                spdlog::warn(
                    "Coroutine {} suspended with args, but arg at index 4 was "
                    "not a string. {}",
                    mDebugName, stackDump);
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

    return { state };
}
} // namespace usagi::scripting::quirrel
