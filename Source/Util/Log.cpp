#include "Log.hpp"

#include <quill/Backend.h>
#include <quill/Frontend.h>
#include <quill/Logger.h>
#include <quill/sinks/FileSink.h>

#include <filesystem>

namespace Acrylic::Log
{
quill::Logger* MainLogger{};

void Init(const std::filesystem::path& logFilePath)
{
    // Start the backend thread
    quill::Backend::start();

    // Setup sink and logger
    auto fileSink = quill::Frontend::create_or_get_sink<quill::FileSink>(
        logFilePath.string(),
        []() {
            quill::FileSinkConfig cfg;
            cfg.set_open_mode('w');
#ifdef RELEASE
            cfg.set_filename_append_option(
                quill::FilenameAppendOption::StartDate);
#endif
            return cfg;
        }(),
        quill::FileEventNotifier{});

    // Create and store the logger
    MainLogger = quill::Frontend::create_or_get_logger(
        "MainLogger",
        std::move(fileSink),
        quill::PatternFormatterOptions{"[%(thread_id)] %(time)"
                                       "%(log_level:^9)"
                                       //"%(short_source_location:<16)"
                                       "%(caller_function:<48)"
                                       "%(message:<64)"
                                       "%(tags)",
                                       "%H:%M:%S.%Qus",
                                       quill::Timezone::LocalTime});

    MainLogger->set_log_level(quill::LogLevel::TraceL3);
}

void Flush()
{
    MainLogger->flush_log();
}
} // namespace Acrylic::Log
