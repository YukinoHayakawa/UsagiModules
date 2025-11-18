#pragma once

#include <string_view>

#include <Usagi/Modules/Scripting/Quirrel/Execution/Execution.hpp>
#include <Usagi/Modules/Scripting/Quirrel/Language/Types.hpp>

namespace usagi::runtime
{
class RuntimeLogger;
} // namespace usagi::runtime

namespace usagi::scripting::quirrel
{
struct RuntimeEnvironment;
} // namespace usagi::scripting::quirrel

namespace usagi::scripting::quirrel::debugging
{
using namespace types;

/*
 * An execution context can implement this interface to better handle
 * information from Quirrel runtime.
 */
class DebuggingInterface
{
public:
    virtual ~DebuggingInterface() = default;

    virtual ExecutionContextHandle get_self_vm() const final;

    virtual void print_func(std::string_view str);
    virtual void error_func(std::string_view str);

    virtual sq_int_t error_handler();

    virtual void compile_error_handler(
        MessageLevels    severity,
        std::string_view desc,
        std::string_view source,
        sq_int_t         line,
        sq_int_t         column,
        std::string_view extra_info
    );

    virtual void debug_hook(
        sq_int_t         event_type,
        std::string_view source_file,
        sq_int_t         line,
        std::string_view func_name
    );

    virtual void log_last_error();
    virtual void print_callstack();

protected:
    virtual ExecutionContextHandle get_self_vm_impl() const = 0;
    virtual RuntimeEnvironment & get_runtime_environment() const = 0;
    virtual runtime::RuntimeLogger & get_logger() const final;
};
} // namespace usagi::scripting::quirrel::debugging
