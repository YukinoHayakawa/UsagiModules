#pragma once

#include <string>

#include <squirrel.h>

#include <sqrat/sqratTypes.h>

#include <Usagi/Modules/Scripting/Quirrel/Config/Defines.hpp>
#include <Usagi/Runtime/ErrorHandling/MaybeError.hpp>
#include <Usagi/Runtime/RAII/RawHandleResource.hpp>

namespace Sqrat
{
class Object;
} // namespace Sqrat

namespace usagi::scripting::quirrel
{
enum class CoroutineExecutionStates
{
    Idle,
    Running,
    Suspended,
};

using CoroutineContext = types::ExecutionContext;
using CoroutineHandle  = types::Handle;

struct CoroutineStates
{
    HSQUIRRELVM      root_machine;
    // The coroutine's execution stack
    CoroutineContext execution_context;
    // Strong reference to the coroutine object
    CoroutineHandle  coroutine_handle;
};

/**
 * @brief Manages a single active Quirrel coroutine.
 *
 * Stores both the VM handle (for execution) and the object handle
 * (for garbage collection management).
 */
class Coroutine : public RawHandleResource<CoroutineStates>
{
public:
    Coroutine(
        HSQUIRRELVM           root_vm,
        std::uint64_t         initial_stack_size,
        std::string           debug_name,
        const Sqrat::Object & coroutine_func);

    Coroutine(Coroutine && other) noexcept             = default;
    Coroutine & operator=(Coroutine && other) noexcept = default;

    CoroutineExecutionStates get_state() const;
    /**
     * \brief Starts the coroutine.
     * Call this ONCE to run the coroutine until its first 'suspend()'.
     */
    SQRESULT start();

    /**
     * \brief Resumes a suspended coroutine.
     * This will push 'null' as the return value for 'suspend()'.
     * \param invoke_err_handler True to invoke the VM's error handler on
     * failure.
     * \return SQRESULT
     */
    SQRESULT resume(bool invoke_err_handler);

    /**
     * \brief Resumes a suspended coroutine, passing a value back.
     * The value will be the return value of 'suspend()' in script.
     * \tparam T Type of the value to push (must be pushable by Sqrat::PushVar)
     * \param value The value to pass to the script.
     * \param invoke_err_handler True to invoke the VM's error handler on
     * failure.
     * \return SQRESULT
     */
    template <typename T>
    SQRESULT resume(T && value, bool invoke_err_handler)
    {
        // This is the "tick" call.
        // The VM MUST be in a 'SUSPENDED' state.

        // Push the provided value (e.g., delta-time)
        Sqrat::PushVar(thread_context(), std::forward<T>(value));

        // resumedret=true: Use the value we just pushed.
        return sq_wakeupvm(
            thread_context(), SQTrue, SQFalse, invoke_err_handler, SQFalse);
    }

    runtime::MaybeError<std::string, CoroutineExecutionStates>
        try_get_yielded_command();

    std::string debug_name() const { return mDebugName; }

    CoroutineContext thread_context() const
    {
        return GetRawHandle().execution_context;
    }

    const CoroutineHandle & coroutine_handle() const
    {
        return TryGetRawHandle().value().get().coroutine_handle;
    }

protected:
    static CoroutineStates CreateCoroutine(
        HSQUIRRELVM           root_vm,
        std::uint64_t         initial_stack_size,
        const Sqrat::Object & coroutine_func);

    std::string mDebugName; // Debug name
};

static_assert(std::is_move_constructible_v<Coroutine>);
} // namespace usagi::scripting::quirrel
