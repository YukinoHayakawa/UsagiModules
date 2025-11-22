#pragma once

#include <format>
#include <memory>
#include <string>
#include <string_view>

#include "LoggingLevels.hpp"

namespace spdlog
{
class logger;
class formatter;

namespace sinks
{
class sink;
} // namespace sinks

namespace level
{
enum level_enum : int;
} // namespace level
} // namespace spdlog

// #include <Usagi/Library/Meta/Reflection/StaticReflection.hpp>

namespace usagi::runtime
{
// Shio: A dedicated logger class that wraps spdlog functionality.
// The logger doesn't have to be Noncopyable since the `unique_ptr` already
// prevents that.
class RuntimeLogger
{
public:
    // Shio: Constructor initializes the logger with a given name.
    // Shio: Note: The logger is created without any sinks. You must add
    // Shio: at least one sink to get output.
    explicit RuntimeLogger(
        std::string name = "default", LoggingLevels level = LoggingLevels::Trace
    );
    // Shio: Destructor handles logger shutdown.
    virtual ~RuntimeLogger();

    // Shio: Add a console sink to the logger.
    void add_console_sink(LoggingLevels level = LoggingLevels::Trace);
    // Shio: Add a basic file sink to the logger.
    void add_file_sink(
        std::string filename, LoggingLevels level = LoggingLevels::Trace
    );
    // Shio: Add a rotating file sink to the logger.
    void add_rotating_file_sink(
        std::string   filename,
        std::size_t   max_file_size,
        std::size_t   max_files,
        bool          rotate_on_open = true,
        LoggingLevels level          = LoggingLevels::Trace
    );

    virtual void set_verbosity(LoggingLevels level);

    // Shio: Log a message with trace level.
    template <typename... Args>
    void trace(std::format_string<Args...> fmt, Args &&... args)
    {
        _do_log_variadic(
            LoggingLevels::Trace, fmt, std::forward<Args>(args)...
        );
    }

    // Shio: Log a message with debug level.
    template <typename... Args>
    void debug(std::format_string<Args...> fmt, Args &&... args)
    {
        _do_log_variadic(
            LoggingLevels::Debug, fmt, std::forward<Args>(args)...
        );
    }

    // Shio: Log a message with info level.
    template <typename... Args>
    void info(std::format_string<Args...> fmt, Args &&... args)
    {
        _do_log_variadic(LoggingLevels::Info, fmt, std::forward<Args>(args)...);
    }

    // Shio: Log a message with warn level.
    template <typename... Args>
    void warn(std::format_string<Args...> fmt, Args &&... args)
    {
        _do_log_variadic(
            LoggingLevels::Warning, fmt, std::forward<Args>(args)...
        );
    }

    // Shio: Log a message with error level.
    template <typename... Args>
    void error(std::format_string<Args...> fmt, Args &&... args)
    {
        _do_log_variadic(
            LoggingLevels::Error, fmt, std::forward<Args>(args)...
        );
    }

    // Shio: Log a message with critical level.
    template <typename... Args>
    void critical(std::format_string<Args...> fmt, Args &&... args)
    {
        _do_log_variadic(
            LoggingLevels::Critical, fmt, std::forward<Args>(args)...
        );
    }

    // Shio: Log a message with custom level.
    template <typename... Args>
    void
        log(LoggingLevels               level,
            std::format_string<Args...> fmt,
            Args &&... args)
    {
        _do_log_variadic(level, fmt, std::forward<Args>(args)...);
    }

protected:
    // todo: metafunctions must be defined in the header so that the compiler
    //   can see them when performing substitution with splicers.
    // static consteval std::meta::info _get_logger_def();
    // static consteval std::meta::info _get_logger_log_func();

    bool _should_log(LoggingLevels level) const;

    // we must not include `spdlog/spdlog.h` here because it will include
    // `<Windows.h>` which pollutes the global namespace.
    template <typename... Args>
    void _do_log_variadic(
        LoggingLevels level, std::format_string<Args...> fmt, Args &&... args
    )
    {
        /* so this won't work because `spdlog::logger` is incomplete here
        static_cast<[:_get_logger_def():] *>(mLogger.get())
            -> [:_get_logger_log_func():](
            level, fmt, std::forward<Args>(args)...
        );
        */
        if(_should_log(level))
        {
            _do_log(level, std::format(fmt, std::forward<Args>(args)...));
        }
    }

    void _do_log(LoggingLevels level, std::string_view str);
    static constexpr spdlog::level::level_enum
        _cast_to_spdlog_level(LoggingLevels level);

    std::unique_ptr<spdlog::logger> mLogger;
};
} // namespace usagi::runtime
