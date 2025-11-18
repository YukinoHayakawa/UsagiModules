#pragma once

#include <memory>
#include <string>

#include <squirrel.h>

#include <sqrat/sqratObject.h>
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
/**
 * \brief Contains the essential handles and metadata for a managed coroutine.
 * \details A Quirrel coroutine (or 'thread') has a dual nature from the C++
 * host's perspective: it is both an executable VM (HSQUIRRELVM) and a garbage-
 * collected object (HSQOBJECT). This struct holds both, along with the root VM
 * it belongs to and other contextual information.
 */
struct CoroutineStates
{
    // Shio: A shared pointer to the root VM from which this coroutine was
    // spawned.
    std::shared_ptr<VirtualMachine> root_machine;
    // Shio: The executable VM handle for this coroutine. All execution
    // functions (sq_call, sq_wakeupvm) operate on this handle.
    CoroutineContext                execution_context;
    // Shio: A strong C++ reference to the coroutine *object*. This handle is
    // given to the garbage collector via sq_addref/sq_release to prevent the
    // coroutine from being collected while the C++ host holds it.
    CoroutineHandle                 coroutine_handle;
    // Shio: The script function that serves as the coroutine's entry point.
    Sqrat::Object                   coroutine_func;
    // Shio: The 'this' instance that the coroutine function is bound to.
    SQObject                        context_instance;
    // Shio: A human-readable name for debugging purposes.
    std::string                     debug_name;
};

/**
 * \brief Manages the lifecycle of a single Quirrel coroutine ('thread').
 *
 * \details This class provides a high-level C++ interface for the complex
 * process of creating, starting, resuming, and destroying a Quirrel coroutine.
 * It correctly handles the dual-handle nature of threads (execution context vs.
 * GC object) and encapsulates the tricky API differences between the initial
 * call and subsequent resumes.
 *
 * --- The Correct Coroutine Lifecycle ---
 * 1.  **Creation**: A `Coroutine` object is constructed, which calls
 *     `sq_newthread` to create a new VM and acquires the necessary handles.
 *     The coroutine is now in the `Idle` state.
 * 2.  **Start**: `start()` is called ONCE. This uses `sq_call` to perform the
 *     initial invocation. The coroutine runs until it hits its first
 *     `suspend()`. It is now in the `Suspended` state. This initial call has
 *     unique stack-handling requirements.
 * 3.  **Resume**: `resume()` is called for all subsequent frames. This uses
 *     `sq_wakeupvm`, which is the correct, clean way to resume a suspended
 *     thread. The coroutine runs until it `suspend()`s again.
 * 4.  **Completion**: When the coroutine function returns, its state changes
 *     back to `Idle`. `resume()` will then fail if called again.
 * 5.  **Destruction**: When the C++ `Coroutine` object is destroyed, it
 *     releases its strong reference, allowing the Quirrel GC to collect the
 *     thread object.
 */
class Coroutine : public RawHandleResource<CoroutineStates>
{
    // We are managing a large object, we don't want to get it copied so often.
    static_assert(return_handle_by_copy_v == false);

public:
    /**
     * \brief Constructs and initializes a new coroutine.
     * \param root_vm The main VM from which the coroutine is spawned.
     * \param initial_stack_size_override Overrides the default stack size.
     * \param debug_name A name for logging and debugging.
     * \param coroutine_func The script function to be executed.
     */
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

    /**
     * \brief Gets the raw HSQUIRRELVM handle for this coroutine.
     * \return The coroutine's execution context.
     */
    CoroutineContext thread_context() const
    {
        return GetRawHandle().execution_context;
    }

    /**
     * \brief Gets the raw HSQOBJECT handle for this coroutine.
     * \return The coroutine's garbage-collected object handle.
     */
    const CoroutineHandle & coroutine_handle() const
    {
        return GetRawHandle().coroutine_handle;
    }

    const Sqrat::Object & coroutine_func() const
    {
        return GetRawHandle().coroutine_func;
    }

    SQObject context_instance() const
    {
        return GetRawHandle().context_instance;
    }

    /**
     * \brief Gets the current execution state of the coroutine.
     * \return "idle", "running", or "suspended".
     */
    ThreadExecutionStates get_execution_state() const;

    /**
     * \brief Starts the coroutine for the first time.
     * \details This must be called only once. It uses `sq_call` to perform the
     * initial invocation, which has special stack handling rules upon
     * suspension. After this, `resume()` should be used for all subsequent
     * executions.
     * \return A SQRESULT indicating success or failure.
     */
    SQRESULT start();

    /**
     * \brief Resumes a suspended coroutine.
     * \details This uses `sq_wakeupvm` to continue execution. It passes 'null'
     * as the return value to the script's `suspend()` call. This should be
     * called for every frame after `start()` has been called once.
     * \param invoke_err_handler True to invoke the VM's error handler on
     * failure.
     * \return A SQRESULT indicating success or failure.
     */
    SQRESULT resume(bool invoke_err_handler = true);

    /**
     * \brief Resumes a suspended coroutine, passing a value back to the script.
     * \details This uses `sq_wakeupvm` to continue execution. The provided
     * `value` becomes the return value of the `suspend()` call within the
     * script, enabling C++ to script communication.
     * \tparam T Type of the value to push (must be pushable by Sqrat::PushVar)
     * \param value The value to pass to the script.
     * \param invoke_err_handler True to invoke the VM's error handler on
     * failure.
     * \return A SQRESULT indicating success or failure.
     */
    template <typename T>
    SQRESULT resume(T && value, bool invoke_err_handler = true)
    {
        // The roottable is always at the bottom of the stack.
        // if(mExecutionFrame == 1) sq_pushroottable(thread_context());

        // This is the "tick" call.
        // The VM MUST be in a 'SUSPENDED' state.
        // sq_pushnull(thread_context());
        _internal_push_instance();

        // Push the provided value (e.g., delta-time)
        Sqrat::PushVar(thread_context(), std::forward<T>(value));

        // resumedret=true: Use the value we just pushed.
        return _internal_resume(false, true, invoke_err_handler);
    }

    /**
     * \brief Attempts to get the value(s) yielded by a `suspend()` call.
     * \details This function should be called after `start()` or `resume()`
     * returns and `get_execution_state()` is `Suspended`. It implements the
     * convention-based approach to retrieving yielded values.
     * \return A string if the first yielded value was a string, otherwise the
     * execution state.
     */
    runtime::MaybeError<std::string, ThreadExecutionStates>
        try_get_yielded_values();

protected:
    // Shio: Internal creation logic called by the constructor.
    static CoroutineStates CreateCoroutine(
        std::shared_ptr<VirtualMachine> root_vm,
        SQInteger                       initial_stack_size_override,
        std::string                     debug_name,
        const Sqrat::Object &           coroutine_func
    );

    // Shio: Pushes the coroutine's 'this' instance onto its own stack.
    void _internal_push_instance();

    // Shio: Core resume logic wrapping `sq_wakeupvm`.
    SQRESULT _internal_resume(
        bool push_this, bool resumed_ret, bool invoke_err_handler
    );

    // Shio: Frame counter for debugging.
    std::uint64_t mExecutionFrame = 0;
};

static_assert(std::is_move_constructible_v<Coroutine>);
} // namespace usagi::scripting::quirrel
