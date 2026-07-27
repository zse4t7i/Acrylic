#pragma once

#include <quill/LogMacros.h>
#include <quill/Logger.h>

// More info at
// https://github.com/odygrd/quill/blob/master/examples/recommended_usage/recommended_usage.cpp

#define LOG_DEBUG(fmt, ...)                                                    \
    QUILL_LOG_DEBUG(Acrylic::Engine::Log::AcrylicLogger, fmt, ##__VA_ARGS__)
#define LOG_INFO(fmt, ...)                                                     \
    QUILL_LOG_INFO(Acrylic::Engine::Log::AcrylicLogger, fmt, ##__VA_ARGS__)
#define LOG_WARNING(fmt, ...)                                                  \
    QUILL_LOG_WARNING(Acrylic::Engine::Log::AcrylicLogger, fmt, ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...)                                                    \
    QUILL_LOG_ERROR(Acrylic::Engine::Log::AcrylicLogger, fmt, ##__VA_ARGS__)

namespace Acrylic::Engine::Log
{
//==============================================================================
// External Variable
//==============================================================================
extern quill::Logger* AcrylicLogger;
//==============================================================================
// External Function
//==============================================================================
void Init(const Path& logPath);
void Flush();
} // namespace Acrylic::Engine::Log
