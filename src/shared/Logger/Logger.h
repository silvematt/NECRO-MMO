#ifndef NECRO_LOGGER_H
#define NECRO_LOGGER_H

#include <iostream>
#include <string>
#include <mutex>

#define FMT_HEADER_ONLY
#include <fmt/core.h>
#include <fmt/format.h>

#include "Utility.h"

namespace NECRO
{
    //---------------------------------------------------------------------------
    // Simple abstract Logger class, used by Console/File Loggers implementations
    //---------------------------------------------------------------------------
    class Logger
    {
    public:
        enum class LogLevel
        {
            LOG_LEVEL_INFO = 0,
            LOG_LEVEL_DEBUG,
            LOG_LEVEL_OKSTATUS,
            LOG_LEVEL_WARNING,
            LOG_LEVEL_ERROR,
            LOG_LEVEL_CRITICAL
        };

    protected:
        std::mutex m_logMutex;

        std::string GetLogLevelStr(LogLevel level);

    public:
        virtual void Log(const std::string& message, LogLevel level, const char* file, int line, ...) = 0;

        template<typename... Args>
        void LogFmt(LogLevel level, const char* file, int line, fmt::format_string<Args...> fmtStr, Args&&... args)
        {
            std::string message = fmt::format(fmtStr, std::forward<Args>(args)...);
            Log(message, level, file, line);
        }

        /*
        // Helper function to format the string using variadic arguments
        // Not used anymore since FMT
        std::string FormatString(const char* str, va_list args)
        {
            // The size of the formatted string, we'll grow it dynamically as needed.
            size_t size = std::vsnprintf(nullptr, 0, str, args) + 1;  // +1 for null terminator
            std::string result(size, '\0');
            std::vsnprintf(&result[0], size, str, args);  // Write the formatted string into the result
            return result;
        } 
        */

        #define cLog ConsoleLogger::Instance()
        #define fLog FileLogger::Instance()

        #define LOG_FMT(logger, level, ...) (logger)->LogFmt(level, __FILE__, __LINE__, __VA_ARGS__)
        // #define LOG(logger, level, message, ...) (logger)->Log(message, level, __FILE__, __LINE__, ##__VA_ARGS__)

        // LOG uses the default Loggers instances, cLog and fLog (consoleLog, fileLog), by default logging on the console will also log on the file
        #define LOG_INFO(message, ...) LOG_FMT(cLog, Logger::LogLevel::LOG_LEVEL_INFO, message, ##__VA_ARGS__); LOG_FMT(fLog, Logger::LogLevel::LOG_LEVEL_INFO, message, ##__VA_ARGS__)
        #define LOG_OK(message, ...) LOG_FMT(cLog, Logger::LogLevel::LOG_LEVEL_OKSTATUS, message, ##__VA_ARGS__); LOG_FMT(fLog, Logger::LogLevel::LOG_LEVEL_OKSTATUS, message, ##__VA_ARGS__)
        #define LOG_DEBUG(message, ...) LOG_FMT(cLog, Logger::LogLevel::LOG_LEVEL_DEBUG, message, ##__VA_ARGS__); LOG_FMT(fLog, Logger::LogLevel::LOG_LEVEL_DEBUG, message, ##__VA_ARGS__)
        #define LOG_WARNING(message, ...) LOG_FMT(cLog, Logger::LogLevel::LOG_LEVEL_WARNING, message, ##__VA_ARGS__); LOG_FMT(fLog, Logger::LogLevel::LOG_LEVEL_WARNING, message, ##__VA_ARGS__)
        #define LOG_ERROR(message, ...) LOG_FMT(cLog, Logger::LogLevel::LOG_LEVEL_ERROR, message, ##__VA_ARGS__); LOG_FMT(fLog, Logger::LogLevel::LOG_LEVEL_ERROR, message, ##__VA_ARGS__)
        #define LOG_CRITICAL(message, ...) LOG_FMT(cLog, Logger::LogLevel::LOG_LEVEL_CRITICAL, message, ##__VA_ARGS__); LOG_FMT(fLog, Logger::LogLevel::LOG_LEVEL_CRITICAL, message, ##__VA_ARGS__)

        // S(PECIFIC) Log, allows to call log on a specific Logger object, may be useful for Daily loggers
        #define SLOG(logger, level, message, ...) (logger).Log(message, level, __FILE__, __LINE__, ##__VA_ARGS__)
        #define SLOG_INFO(logger, message, ...) LOG_FMT(logger, Logger::LogLevel::LOG_LEVEL_INFO, message, ##__VA_ARGS__)
        #define SLOG_OK(logger, message, ...) LOG_FMT(logger, Logger::LogLevel::LOG_LEVEL_OKSTATUS, message, ##__VA_ARGS__)
        #define SLOG_DEBUG(logger, message, ...) LOG_FMT(logger, Logger::LogLevel::LOG_LEVEL_DEBUG, message, ##__VA_ARGS__)
        #define SLOG_WARNING(logger, message, ...) LOG_FMT(logger, Logger::LogLevel::LOG_LEVEL_WARNING, message, ##__VA_ARGS__)
        #define SLOG_ERROR(logger, message, ...) LOG_FMT(logger, Logger::LogLevel::LOG_LEVEL_ERROR, message, ##__VA_ARGS__)
        #define SLOG_CRITICAL(logger, message, ...) LOG_FMT(logger, Logger::LogLevel::LOG_LEVEL_CRITICAL, message, ##__VA_ARGS__)
    };

}

#endif
