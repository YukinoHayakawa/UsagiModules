#pragma once

#include <Usagi/Modules/Scripting/Quirrel/Execution/Execution.hpp>
#include <Usagi/Modules/Scripting/Quirrel/Language/Types.hpp>

namespace usagi::scripting::quirrel::debugging
{
using namespace types;

class DebuggingInterface;

/*
 * Dispatchers to actual handlers of Quirrel VMs.
 */
struct DebuggingCommon
{
    // todo configurable buffer size.
    constexpr static std::size_t print_buffer_size_v = 1'024;

    static void
        print_func(ExecutionContextHandle vm, const sq_char_t * str, ...);
    static void
        error_func(ExecutionContextHandle vm, const sq_char_t * str, ...);

    static sq_int_t error_handler(ExecutionContextHandle vm);

    static void compile_error_handler(
        ExecutionContextHandle vm,
        MessageLevels          severity,
        const sq_char_t *      desc,
        const sq_char_t *      source,
        sq_int_t               line,
        sq_int_t               column,
        const sq_char_t *      extra_info
    );

    // Advanced debugging hook
    static void debug_hook(
        ExecutionContextHandle vm,
        sq_int_t               event_type,
        const sq_char_t *      source_file,
        sq_int_t               line,
        const sq_char_t *      func_name
    );

    static void bind_handlers_to(ExecutionContextHandle vm);

    static DebuggingInterface *
        get_debugging_interface(ExecutionContextHandle vm);
};
} // namespace usagi::scripting::quirrel::debugging
