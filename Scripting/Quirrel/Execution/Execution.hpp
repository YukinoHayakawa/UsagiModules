#pragma once

#include <type_traits>

struct SQVM;
using HSQUIRRELVM = std::add_pointer_t<SQVM>;

namespace usagi::scripting::quirrel
{
using ExecutionContextHandle = HSQUIRRELVM;

/*
 * This enum corresponds to `SQ_VMSTATE_...` from `squirrel.h`. It is
 * universally applicable to any execution contexts from the root vm to threads.
 * Both root Squirrel VM and Coroutine threads will use this as their state
 * indicators.
 */
enum class ThreadExecutionStates : std::uint8_t
{
    Idle      = 0,
    Running   = 1,
    Suspended = 2,
};
} // namespace usagi::scripting::quirrel
