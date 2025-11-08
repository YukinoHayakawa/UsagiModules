#pragma once

#include <string>

#include <squirrel.h>

#include <Usagi/Library/Memory/Noncopyable.hpp>
#include <Usagi/Modules/Scripting/Quirrel/Config/Defines.hpp>
#include <Usagi/Runtime/ErrorHandling/MaybeError.hpp>

namespace Sqrat
{
class Object;
} // namespace Sqrat

namespace usagi::scripting::quirrel
{
enum class CoroutineStates
{
    Idle,
    Running,
    Suspended,
};

using CoroutineContext = types::ExecutionContext;
using CoroutineHandle  = types::Handle;

/**
 * @brief Manages a single active Quirrel coroutine.
 *
 * Stores both the VM handle (for execution) and the object handle
 * (for garbage collection management).
 */
class Coroutine : public Noncopyable
{
public:
    Coroutine(
        HSQUIRRELVM           root_vm,
        std::uint64_t         initial_stack_size,
        std::string           debug_name,
        const Sqrat::Object & coroutine_func);
    virtual ~Coroutine();

    Coroutine(Coroutine && other) noexcept             = default;
    Coroutine & operator=(Coroutine && other) noexcept = default;

    CoroutineStates get_state() const;
    SQRESULT resume(bool ret_value, bool invoke_err_handler = true);
    runtime::MaybeError<std::string, CoroutineStates> try_get_yielded_command();

    std::string debug_name() const { return mDebugName; }

    CoroutineContext thread_context() const { return mThreadExecution; }

    CoroutineHandle coroutine_handle() const { return mCoroutineHandle; }

protected:
    HSQUIRRELVM      mRootMachine;
    CoroutineContext mThreadExecution; // The coroutine's execution stack
    CoroutineHandle
                mCoroutineHandle; // Strong reference to the coroutine object
    std::string mDebugName;       // Debug name
};
static_assert(std::is_move_constructible_v<Coroutine>);
} // namespace usagi::scripting::quirrel
