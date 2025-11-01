#include "RuntimeLogger.hpp"

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

namespace usagi::runtime
{
RuntimeLogger::RuntimeLogger(const std::string &name)
{
    // Shio: Create a logger with the given name.
    // Shio: The logger is created without any sinks. You must add at least one
    // Shio: sink to get output.
    mLogger = std::make_unique<spdlog::logger>(name);
    // Shio: Set the logging level to trace by default.
    mLogger->set_level(spdlog::level::trace);
    // Shio: Set the log format to a kernel-style output.
    mLogger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v");
    mLogger->info("Logger '{}' initialized.", name);
}

RuntimeLogger::~RuntimeLogger()
{
    // Shio: Drop the logger to release resources.
    spdlog::drop(mLogger->name());
}

void RuntimeLogger::AddConsoleSink(spdlog::level::level_enum level)
{
    // Shio: Create a console sink with color output.
    auto sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    sink->set_level(level);
    mLogger->sinks().push_back(sink);
}

void RuntimeLogger::AddFileSink(
    const std::string        &filename,
    spdlog::level::level_enum level)
{
    // Shio: Create a basic file sink.
    auto sink =
        std::make_shared<spdlog::sinks::basic_file_sink_mt>(filename, true);
    sink->set_level(level);
    mLogger->sinks().push_back(sink);
}

void RuntimeLogger::AddRotatingFileSink(
    const std::string        &filename,
    std::size_t               max_file_size,
    std::size_t               max_files,
    bool                      rotate_on_open,
    spdlog::level::level_enum level)
{
    // Shio: Create a rotating file sink.
    auto sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
        filename,
        max_file_size,
        max_files,
        rotate_on_open);
    sink->set_level(level);
    mLogger->sinks().push_back(sink);
}
} // namespace usagi::runtime
