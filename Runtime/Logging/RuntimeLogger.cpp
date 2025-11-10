#include "RuntimeLogger.hpp"

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

namespace usagi::runtime
{
RuntimeLogger::RuntimeLogger(std::string name, spdlog::level::level_enum level)
{
    // Shio: Create a logger with the given name.
    // Shio: The logger is created without any sinks. You must add at least one
    // Shio: sink to get output.
    mLogger = std::make_unique<spdlog::logger>(std::move(name));
    // Shio: Set the logging level to trace by default.
    mLogger->set_level(level);
    // Shio: Set the log format to a kernel-style output.
    mLogger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v");
    mLogger->info("Logger '{}' initialized.", mLogger->name());
}

RuntimeLogger::~RuntimeLogger()
{
    // Shio: Drop the logger to release resources.
    spdlog::drop(mLogger->name());
}

void RuntimeLogger::add_console_sink(spdlog::level::level_enum level)
{
    // Shio: Create a console sink with color output.
    auto sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    sink->set_level(level);
    mLogger->sinks().push_back(sink);
}

void RuntimeLogger::add_file_sink(
    std::string filename, spdlog::level::level_enum level
)
{
    // Shio: Create a basic file sink.
    auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(
        std::move(filename), true
    );
    sink->set_level(level);
    mLogger->sinks().push_back(sink);
}

void RuntimeLogger::add_rotating_file_sink(
    std::string               filename,
    std::size_t               max_file_size,
    std::size_t               max_files,
    bool                      rotate_on_open,
    spdlog::level::level_enum level
)
{
    // Shio: Create a rotating file sink.
    auto sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
        std::move(filename), max_file_size, max_files, rotate_on_open
    );
    sink->set_level(level);
    mLogger->sinks().push_back(sink);
}

void RuntimeLogger::set_verbosity(spdlog::level::level_enum level)
{
    mLogger->set_level(level);
}

bool RuntimeLogger::_should_log(spdlog::level::level_enum level) const
{
    return mLogger->should_log(level);
}

void RuntimeLogger::_do_log(
    spdlog::level::level_enum level, std::string_view str
)
{
    mLogger->log(level, str);
}

/*
consteval std::meta::info RuntimeLogger::_get_logger_def()
{
    return ^^spdlog::logger;
}

consteval std::meta::info RuntimeLogger::_get_logger_log_func()
{
    return ^^spdlog::logger::log;
}
*/
} // namespace usagi::runtime
