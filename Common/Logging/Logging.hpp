﻿#pragma once

#include <string_view>

#include "Format.hpp"

namespace usagi::logging
{
enum class LoggingLevel
{
    trace = 0,
    debug = 1,
    info = 2,
    warn = 3,
    error = 4,
    critical = 5,
    off = 6
};

bool should_log(LoggingLevel level);
void do_log(LoggingLevel level, std::string_view msg);
void add_file_sink(const std::string &file_path);

/**
 * \brief
 * \tparam Args
 * \param level
 * \param fmt For consistency, always treat fmt as a format string even when no
 * arg is present. Be aware when using strings directly passed from other
 * sources since they may contain formatting tokens.
 * \param args
 */
template <typename... Args>
void log(const LoggingLevel level, std::string_view fmt, Args &&... args)
{
    if(!should_log(level)) return;

    fmt::memory_buffer buffer;
    fmt::format_to(buffer, fmt, std::forward<Args>(args)...);
    do_log(level, { buffer.data(), buffer.size() });
}
}

#define LOG(level, ...) \
    ::usagi::logging::log(\
        ::usagi::logging::LoggingLevel::level, __VA_ARGS__) \
/**/