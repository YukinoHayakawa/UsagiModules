#include "Logger.hpp"

#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

namespace usagi::runtime
{
Logger::Logger(const std::string &name)
{
    // Shio: Create a console logger with color output.
    m_logger = spdlog::stdout_color_mt(name);
    // Shio: Set the logging level to debug by default.
    m_logger->set_level(spdlog::level::debug);
    m_logger->info("Logger '{}' initialized.", name);
}

Logger::~Logger()
{
    // Shio: Drop the logger to release resources.
    spdlog::drop(m_logger->name());
}
} // namespace usagi::runtime
