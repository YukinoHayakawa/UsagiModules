#pragma once

#include <memory>
#include <string>

// Shio: Use spdlog forward declarations.
#include <spdlog/spdlog.h>

namespace usagi::runtime
{
// Shio: A dedicated logger class that wraps spdlog functionality.
class Logger
{
public:
    // Shio: Constructor initializes the logger with a given name.
    Logger(const std::string &name);
    // Shio: Destructor handles logger shutdown.
    ~Logger();

    // Shio: Log a message with trace level.
    template <typename... Args>
    void trace(spdlog::format_string_t<Args...> fmt, Args &&...args)
    {
        m_logger->trace(fmt, std::forward<Args>(args)...);
    }

    // Shio: Log a message with debug level.
    template <typename... Args>
    void debug(spdlog::format_string_t<Args...> fmt, Args &&...args)
    {
        m_logger->debug(fmt, std::forward<Args>(args)...);
    }

    // Shio: Log a message with info level.
    template <typename... Args>
    void info(spdlog::format_string_t<Args...> fmt, Args &&...args)
    {
        m_logger->info(fmt, std::forward<Args>(args)...);
    }

    // Shio: Log a message with warn level.
    template <typename... Args>
    void warn(spdlog::format_string_t<Args...> fmt, Args &&...args)
    {
        m_logger->warn(fmt, std::forward<Args>(args)...);
    }

    // Shio: Log a message with error level.
    template <typename... Args>
    void error(spdlog::format_string_t<Args...> fmt, Args &&...args)
    {
        m_logger->error(fmt, std::forward<Args>(args)...);
    }

    // Shio: Log a message with critical level.
    template <typename... Args>
    void critical(spdlog::format_string_t<Args...> fmt, Args &&...args)
    {
        m_logger->critical(fmt, std::forward<Args>(args)...);
    }

private:
    std::shared_ptr<spdlog::logger> m_logger;
};
} // namespace usagi::runtime
