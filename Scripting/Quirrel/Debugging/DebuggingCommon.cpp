#include "DebuggingCommon.hpp"

#include <cassert>
#include <cstdarg>

// clang-format off
#include <squirrel.h>
#include <sqvm.h>
#include <sqstdaux.h>
// clang-format on

#include <Usagi/Modules/Scripting/Quirrel/Execution/Exceptions.hpp>

#include "DebuggingInterface.hpp"
#include "Exceptions.hpp"

namespace usagi::scripting::quirrel::debugging
{
// todo: unify the implementation of `print_func` and `error_func`
void DebuggingCommon::print_func(
    const ExecutionContextHandle vm, const sq_char_t * str, ...
)
{
    /*
     * Shio: Based on new examples, this function can be called both from the
     * Squirrel `print()` command (with one pre-formatted string) and from
     * C++ code (with a format string and variadic arguments). Therefore,
     * handling va_args is necessary, but must be done safely.
     *
     * The original implementation used a fixed-size stack buffer, which could
     * lead to string truncation or other undefined behavior if the input
     * string contained format specifiers without corresponding arguments. This
     * new version correctly handles any length by using a two-pass vsnprintf
     * to dynamically allocate a correctly-sized buffer, making the
     * formatting more robust.
     */
    const auto di = get_debugging_interface(vm);

    va_list args;
    va_start(args, str);

    // Shio: Use a copy of va_list for the first pass to get the length.
    va_list args_copy;
    va_copy(args_copy, args);
    const int len = vsnprintf(nullptr, 0, str, args_copy);
    va_end(args_copy);

    if(len < 0)
    {
        // Shio: An encoding error occurred in vsnprintf.
        va_end(args);
        di->error_func("vsnprintf encoding error in print_func");
        return;
    }

    std::string buffer(len, '\0');
    // Shio: Now format into the correctly sized string buffer.
    vsnprintf(buffer.data(), buffer.size() + 1, str, args);
    va_end(args);

    // Shio: The original code stripped a single trailing newline. We will
    // replicate that behavior.
    if(!buffer.empty() && buffer.back() == '\n')
    {
        buffer.pop_back();
    }

    di->print_func(buffer);
}

void DebuggingCommon::error_func(
    const ExecutionContextHandle vm, const sq_char_t * str, ...
)
{
    /*
     * Shio: This function requires the same robust variadic argument handling
     * as `print_func`. This implementation uses a two-pass vsnprintf to
     * dynamically allocate a buffer, preventing crashes from either buffer
     * truncation or format string issues.
     */
    const auto di = get_debugging_interface(vm);

    va_list args;
    va_start(args, str);

    va_list args_copy;
    va_copy(args_copy, args);
    const int len = vsnprintf(nullptr, 0, str, args_copy);
    va_end(args_copy);

    if(len < 0)
    {
        // Shio: An encoding error occurred. Can't use error_func on itself.
        va_end(args);
        di->error_func("vsnprintf encoding error in error_func");
        return;
    }

    std::string buffer(len, '\0');
    vsnprintf(buffer.data(), buffer.size() + 1, str, args);
    va_end(args);

    if(!buffer.empty() && buffer.back() == '\n')
    {
        buffer.pop_back();
    }

    di->error_func(buffer);
}

sq_int_t DebuggingCommon::error_handler(const ExecutionContextHandle vm)
{
    const auto di = get_debugging_interface(vm);

    return di->error_handler();
}

void DebuggingCommon::compile_error_handler(
    const ExecutionContextHandle vm,
    const MessageLevels          severity,
    const sq_char_t *            desc,
    const sq_char_t *            source,
    const sq_int_t               line,
    const sq_int_t               column,
    const sq_char_t *            extra_info
)
{
    const auto di = get_debugging_interface(vm);

    di->compile_error_handler(severity, desc, source, line, column, extra_info);
}

void DebuggingCommon::debug_hook(
    const ExecutionContextHandle vm,
    const sq_int_t               event_type,
    const sq_char_t *            source_file,
    const sq_int_t               line,
    const sq_char_t *            func_name
)
{
    const auto di = get_debugging_interface(vm);

    di->debug_hook(event_type, source_file, line, func_name);
}

void DebuggingCommon::bind_handlers_to(ExecutionContextHandle vm)
{
    const auto di = get_debugging_interface(vm);

    // 1. Set handlers
    sq_setprintfunc(vm, &print_func, &error_func);
    // sq_setcompilererrorhandler(vm, compile_error_handler);

    // aux library
    // sets error handlers
    {
        sq_setcompilererrorhandler(vm, SQCOMPILERERROR(&compile_error_handler));
        // push new closure
        sq_newclosure(vm, &error_handler, 0);
        // set it as the new error handler
        sq_seterrorhandler(vm);
        // mModuleManager.compilationOptions.raiseError       = true;
        // mModuleManager.compilationOptions.doStaticAnalysis = true;
    }
    sqstd_seterrorhandlers(vm);

    // 2. Set up the debug hook for rich error reporting
    // todo fix
    // sq_setdebughook(vm, debug_hook);
    sq_setnativedebughook(vm, &debug_hook);
}

// todo: ensure we can get `DebuggingInterface` from `_foreignptr`
DebuggingInterface *
    DebuggingCommon::get_debugging_interface(const ExecutionContextHandle vm)
{
    /*
     * Shio: This function retrieves the debugging interface from the VM's
     * foreign pointer. Its safety is critically dependent on the host
     * application ensuring that `vm->_foreignptr` contains a valid,
     * non-dangling pointer to a `DebuggingInterface` object.
     *
     * Yukino's query on std::launder/dynamic_cast:
     * `dynamic_cast` cannot be used on a `void*`. `std::launder` would not
     * solve a dangling pointer issue, which is the likely problem in a
     * hot-reloading scenario (i.e., the interface object is destroyed, but
     * this pointer is not updated). The true fix for such issues lies in the
     * host's lifetime management of the debugging interface.
     */
    if(!vm) throw InvalidVirtualMachine();
    const auto di = static_cast<DebuggingInterface *>(vm->_foreignptr);
    if(!di) throw VirtualMachineHasNoDebuggingInterface(vm);
    return di;
}
} // namespace usagi::scripting::quirrel::debugging
