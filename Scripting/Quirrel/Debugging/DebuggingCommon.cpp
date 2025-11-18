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
void DebuggingCommon::print_func(
    const ExecutionContextHandle vm, const sq_char_t * str, ...
)
{
    const auto di = get_debugging_interface(vm);

    char    buffer[print_buffer_size_v];
    va_list args;
    va_start(args, str);
    vsnprintf(buffer, sizeof(buffer), str, args);
    va_end(args);
    if(strlen(buffer) > 0 && buffer[strlen(buffer) - 1] == '\n')
        buffer[strlen(buffer) - 1] = '\0';

    di->print_func(buffer);
}

void DebuggingCommon::error_func(
    const ExecutionContextHandle vm, const sq_char_t * str, ...
)
{
    const auto di = get_debugging_interface(vm);

    char    buffer[print_buffer_size_v];
    va_list args;
    va_start(args, str);
    vsnprintf(buffer, sizeof(buffer), str, args);
    va_end(args);
    if(strlen(buffer) > 0 && buffer[strlen(buffer) - 1] == '\n')
        buffer[strlen(buffer) - 1] = '\0';

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

DebuggingInterface *
    DebuggingCommon::get_debugging_interface(const ExecutionContextHandle vm)
{
    if(!vm) throw InvalidVirtualMachine();
    const auto di = static_cast<DebuggingInterface *>(vm->_foreignptr);
    if(!di) throw VirtualMachineHasNoDebuggingInterface(vm);
    return di;
}
} // namespace usagi::scripting::quirrel::debugging
