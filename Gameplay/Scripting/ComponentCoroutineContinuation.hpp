#pragma once

#include <cstdint>

namespace usagi
{
struct ComponentCoroutineContinuation
{
    std::uint64_t next_entry_point = 0;

    enum ResumeCondition : std::uint8_t
    {
        NEVER,
        USER_INPUT,
        NEXT_FRAME,
    } resume_condition = NEVER;
};
}
