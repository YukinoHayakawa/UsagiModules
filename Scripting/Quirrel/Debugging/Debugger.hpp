#pragma once

#include <cassert>

// clang-format off
#include <squirrel.h>
#include <sqvm.h>
// clang-format on

namespace usagi::scripting::quirrel
{
class Debugger
{
public:
    static void visit_stack_frame(
        const HSQUIRRELVM vm,
        auto &&           op,
        const bool        variadic_func = false,
        const bool        with_instance = false
    )
    {
        const SQInteger stackTop = sq_gettop(vm);
        // The stackframe of a variadic function invocation is bounded by two
        // `[OT_NULL]`, one as the prologue, another as the epilogue. Arguments
        // are between them, so we visit the arguments starting from the second
        // one and ignore the last one.
        const auto begin = (variadic_func ? 2 : 1) + (with_instance ? 1 : 0);
        // Exclude the ending `[OT_NULL]` if variadic.
        const auto end   = variadic_func ? stackTop : stackTop + 1;
        for(SQInteger i = begin; i < end; ++i)
        {
            SQObjectPtr s = stack_get(vm, i);
            op(s);
        }
    }

    static std::string format_stack_frame(HSQUIRRELVM vm);
    static std::string format_stack_values(HSQUIRRELVM vm);
};
} // namespace usagi::scripting::quirrel
