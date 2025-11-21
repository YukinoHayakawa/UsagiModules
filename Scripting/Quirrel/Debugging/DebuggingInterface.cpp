#include "DebuggingInterface.hpp"

#include <squirrel.h>

#include <Usagi/Modules/Runtime/Logging/RuntimeLogger.hpp>
#include <Usagi/Modules/Scripting/Quirrel/Execution/Exceptions.hpp>
#include <Usagi/Modules/Scripting/Quirrel/Language/RuntimeTypes.hpp>
#include <Usagi/Modules/Scripting/Quirrel/RuntimeEnvironment.hpp>

namespace usagi::scripting::quirrel::debugging
{
ExecutionContextHandle DebuggingInterface::get_self_vm() const
{
    const auto vm = get_self_vm_impl();
    if(!vm) throw InvalidVirtualMachine();
    return vm;
}

void DebuggingInterface::print_func(std::string_view str)
{
    get_logger().info("{}", str);
}

void DebuggingInterface::error_func(std::string_view str)
{
    get_logger().error("{}", str);
}

sq_int_t DebuggingInterface::error_handler()
{
    // SQInteger event_type;
    // sq_getinteger(v, 2, &event_type);

    [[maybe_unused]]
    const auto vm = get_self_vm();

    // todo: this is no such an `event_type`
    // if(event_type == 'e') // 'e' for "exception thrown"
    {
        get_logger().critical("--- QUIRREL RUNTIME ERROR ---");
        print_callstack();
    }

    return 0;
}

void DebuggingInterface::compile_error_handler(
    MessageLevels    severity,
    std::string_view desc,
    std::string_view source,
    sq_int_t         line,
    sq_int_t         column,
    std::string_view extra_info
)
{
    get_logger().log(
        [=] {
            switch(severity)
            {

                case MessageLevels::Warning:
                    return runtime::LoggingLevels::Warning;
                case MessageLevels::Error: return runtime::LoggingLevels::Error;
                case MessageLevels::Info :
                default                  : return runtime::LoggingLevels::Info;
            }
        }(),
        "Compile Error in '{}' (line {}, col {}): {} {}", source, line, column,
        desc, extra_info
    );
}

void DebuggingInterface::debug_hook(
    const sq_int_t   event_type,
    std::string_view source_file,
    sq_int_t         line,
    std::string_view func_name
)
{
    if(func_name.empty()) func_name = "<unknown>";
    if(source_file.empty()) source_file = "<unknown>";

    switch(event_type)
    {
        /*case 'l': // called every line(that contains some code)
            vm->get_logger().trace(
                "[STMT_EXEC] LINE line [{0}] func [{1}] file [{2}]", line,
                func_name, source_file
            );
            break;*/
        case 'c': // called when a function has been called
            get_logger().trace(
                "[FUNC_CALL] LINE line [{0}] func [{1}] file [{2}]", line,
                func_name, source_file
            );
            break;
        case 'r': // called when a function returns
            get_logger().trace(
                "[FUNC_RETN] LINE line [{0}] func [{1}] file [{2}]", line,
                func_name, source_file
            );
            break;
    }
}

void DebuggingInterface::log_last_error()
{
    const auto vm = get_self_vm();

    const SQChar * err = nullptr;
    sq_getlasterror(vm);
    if(SQ_SUCCEEDED(sq_getstring(vm, -1, &err)) && err)
    {
        get_logger().error("[QUIRREL RUNTIME] {}", err);
    }
    sq_poptop(vm); // Pop the error
}

// todo: see `sqstd_printcallstack`/`_sqstd_aux_printerror`
void DebuggingInterface::print_callstack()
{
    const auto vm = get_self_vm();

    SQStackInfos si;
    SQInteger    level = 1;
    while(SQ_SUCCEEDED(sq_stackinfos(vm, level, &si)))
    {
        const char * func = si.funcname ? si.funcname : "unknown_function";
        const char * src  = si.source ? si.source : "unknown_source";
        get_logger().error("  [{}] {}() [{}:{}]", level, func, src, si.line);
        level++;
    }
}

runtime::RuntimeLogger & DebuggingInterface::get_logger() const
{
    return get_runtime_environment()
        .service_provider.ensure_service<runtime::RuntimeLogger>();
}
} // namespace usagi::scripting::quirrel::debugging
