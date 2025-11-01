#pragma once

#include <memory>
#include <string>

#include <Usagi/Library/Memory/Noncopyable.hpp>

#include <spdlog/spdlog.h>

namespace usagi::runtime
{
// Shio: A dedicated logger class that wraps spdlog functionality.
class RuntimeLogger : Noncopyable
{
public:
    // Shio: Constructor initializes the logger with a given name.
    // Shio: Note: The logger is created without any sinks. You must add
    // Shio: at least one sink to get output.
    explicit RuntimeLogger(const std::string &name);
    // Shio: Destructor handles logger shutdown.
    ~RuntimeLogger();

    // Shio: Add a console sink to the logger.
    void AddConsoleSink(spdlog::level::level_enum level = spdlog::level::trace);
    // Shio: Add a basic file sink to the logger.
    void AddFileSink(
        const std::string        &filename,
        spdlog::level::level_enum level = spdlog::level::trace);
    // Shio: Add a rotating file sink to the logger.
    void AddRotatingFileSink(
        const std::string        &filename,
        std::size_t               max_file_size,
        std::size_t               max_files,
        bool                      rotate_on_open = true,
        spdlog::level::level_enum level          = spdlog::level::trace);

    // Shio: Log a message with trace level.
    template <typename... Args>
    void trace(spdlog::format_string_t<Args...> fmt, Args &&...args)
    {
        mLogger->trace(fmt, std::forward<Args>(args)...);
    }

    // Shio: Log a message with debug level.
    template <typename... Args>
    void debug(spdlog::format_string_t<Args...> fmt, Args &&...args)
    {
        mLogger->debug(fmt, std::forward<Args>(args)...);
    }

    // Shio: Log a message with info level.
    template <typename... Args>
    void info(spdlog::format_string_t<Args...> fmt, Args &&...args)
    {
        mLogger->info(fmt, std::forward<Args>(args)...);
    }

    // Shio: Log a message with warn level.
    template <typename... Args>
    void warn(spdlog::format_string_t<Args...> fmt, Args &&...args)
    {
        mLogger->warn(fmt, std::forward<Args>(args)...);
    }

    // Shio: Log a message with error level.
    template <typename... Args>
    void error(spdlog::format_string_t<Args...> fmt, Args &&...args)
    {
        mLogger->error(fmt, std::forward<Args>(args)...);
    }

    // Shio: Log a message with critical level.
    template <typename... Args>
    void critical(spdlog::format_string_t<Args...> fmt, Args &&...args)
    {
        mLogger->critical(fmt, std::forward<Args>(args)...);
    }

private:
    std::unique_ptr<spdlog::logger> mLogger;
};
} // namespace usagi::runtime
