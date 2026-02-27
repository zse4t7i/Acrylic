#pragma once

#include <quill/LogMacros.h>
#include <quill/Logger.h>

#include <filesystem>

// More info at
// https://github.com/odygrd/quill/blob/master/examples/recommended_usage/recommended_usage.cpp

#define LOG_DEBUG(fmt, ...)                                                    \
    QUILL_LOG_DEBUG(Acrylic::Log::MainLogger, fmt, ##__VA_ARGS__)
#define LOG_INFO(fmt, ...)                                                     \
    QUILL_LOG_INFO(Acrylic::Log::MainLogger, fmt, ##__VA_ARGS__)
#define LOG_WARNING(fmt, ...)                                                  \
    QUILL_LOG_WARNING(Acrylic::Log::MainLogger, fmt, ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...)                                                    \
    QUILL_LOG_ERROR(Acrylic::Log::MainLogger, fmt, ##__VA_ARGS__)

namespace Acrylic::Log
{
extern quill::Logger* MainLogger;

void Init(const std::filesystem::path& path);

} // namespace Acrylic::Log
