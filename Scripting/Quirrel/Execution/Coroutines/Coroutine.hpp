#pragma once

#include <memory>
#include <string>

#include <squirrel.h>

#include <sqrat/sqratTypes.h>

#include <Usagi/Modules/Scripting/Quirrel/Execution/ThreadExecutionStates.hpp>
#include <Usagi/Modules/Scripting/Quirrel/Language/Types.hpp>
#include <Usagi/Runtime/RAII/RawHandleResource.hpp>

namespace usagi::scripting::quirrel
{
class VirtualMachine;
} // namespace usagi::scripting::quirrel

namespace Sqrat
{
class Object;
} // namespace Sqrat

namespace usagi::scripting::quirrel
{
using CoroutineContext = ExecutionContext;
using CoroutineHandle  = Handle;

struct CoroutineStates
{
    std::shared_ptr<VirtualMachine> root_machine;
    // The coroutine's execution stack
    CoroutineContext                execution_context;
    // Strong reference to the coroutine object
    CoroutineHandle                 coroutine_handle;
    std::string                     debug_name;
};

/**
 * @brief Manages a single active Quirrel coroutine.
 *
 * Stores both the VM handle (for execution) and the object handle
 * (for garbage collection management).
 */
class Coroutine : public RawHandleResource<CoroutineStates>
{
    // We are managing a large object, we don't want to get it copied so often.
    static_assert(return_handle_by_copy_v == false);

public:
    Coroutine(
        std::shared_ptr<VirtualMachine> root_vm,
        SQInteger                       initial_stack_size_override,
        std::string                     debug_name,
        const Sqrat::Object &           coroutine_func
    );

    Coroutine(Coroutine && other) noexcept             = default;
    Coroutine & operator=(Coroutine && other) noexcept = default;

    std::shared_ptr<VirtualMachine> root_vm() const
    {
        return GetRawHandle().root_machine;
    }

    std::string debug_name() const { return GetRawHandle().debug_name; }

    CoroutineContext thread_context() const
    {
        return GetRawHandle().execution_context;
    }

    const CoroutineHandle & coroutine_handle() const
    {
        return GetRawHandle().coroutine_handle;
    }

    ThreadExecutionStates get_execution_state() const;

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
    SQRESULT resume(bool invoke_err_handler = true);

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
    SQRESULT resume(T && value, bool invoke_err_handler = true)
    {
        // The roottable is always at the bottom of the stack.
        // sq_pushroottable(thread_context());

        // This is the "tick" call.
        // The VM MUST be in a 'SUSPENDED' state.
        // sq_pushnull(thread_context());
        _internal_push_instance();

        // Push the provided value (e.g., delta-time)
        Sqrat::PushVar(thread_context(), std::forward<T>(value));

        // resumedret=true: Use the value we just pushed.
        return _internal_resume(false, true, invoke_err_handler);
    }

    runtime::MaybeError<std::string, ThreadExecutionStates>
        try_get_yielded_values();

protected:
    static CoroutineStates CreateCoroutine(
        std::shared_ptr<VirtualMachine> root_vm,
        SQInteger                       initial_stack_size_override,
        std::string                     debug_name,
        const Sqrat::Object &           coroutine_func
    );

    void _internal_push_instance();

    SQRESULT _internal_resume(
        bool push_this, bool resumed_ret, bool invoke_err_handler
    );
};

static_assert(std::is_move_constructible_v<Coroutine>);
} // namespace usagi::scripting::quirrel
