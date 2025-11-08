#pragma once

#include <string>

#include <squirrel.h>

#include <Usagi/Library/Memory/Noncopyable.hpp>
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
    SQRESULT resume(bool ret_value, bool invoke_err_handler = true);
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
