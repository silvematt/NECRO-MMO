#ifndef NECRO_LOGGER_H
#define NECRO_LOGGER_H

#include <iostream>
#include <string>
#include <mutex>

#include "Utility.h"

//---------------------------------------------------------------------------
// Simple abstract Logger class, used by Console/File Loggers implementations
//---------------------------------------------------------------------------
class Logger
{
public:
    enum LogLevel
    {
        LOG_LEVEL_INFO = 0,
        LOG_LEVEL_DEBUG,
        LOG_LEVEL_OKSTATUS,
        LOG_LEVEL_WARNING,
        LOG_LEVEL_ERROR,
        LOG_LEVEL_CRITICAL
    };

    // Helper function to format the string using variadic arguments
    std::string FormatString(const char* str, va_list args)
    {
        // The size of the formatted string, we'll grow it dynamically as needed.
        size_t size = std::vsnprintf(nullptr, 0, str, args) + 1;  // +1 for null terminator
        std::string result(size, '\0');
        std::vsnprintf(&result[0], size, str, args);  // Write the formatted string into the result
        return result;
    }

protected:
    std::mutex lMutex;
    std::string GetLogLevelStr(LogLevel level);

public:
    virtual void Log(const std::string& message, LogLevel level, const char* file, int line, ...) = 0;

#define cLog ConsoleLogger::Instance()
#define fLog FileLogger::Instance()

    // LOG uses the default Loggers instances, cLog and fLog (consoleLog, fileLog), by default logging on the console will also log on the file
#define LOG(logger, level, message, ...) (logger)->Log(message, level, __FILE__, __LINE__, ##__VA_ARGS__)
#define LOG_INFO(message, ...) LOG(cLog, Logger::LogLevel::LOG_LEVEL_INFO, message, ##__VA_ARGS__); LOG(fLog, Logger::LogLevel::LOG_LEVEL_INFO, message, ##__VA_ARGS__)
#define LOG_OK(message, ...) LOG(cLog, Logger::LogLevel::LOG_LEVEL_OKSTATUS, message, ##__VA_ARGS__); LOG(fLog, Logger::LogLevel::LOG_LEVEL_OKSTATUS, message, ##__VA_ARGS__)
#define LOG_DEBUG(message, ...) LOG(cLog, Logger::LogLevel::LOG_LEVEL_DEBUG, message, ##__VA_ARGS__); LOG(fLog, Logger::LogLevel::LOG_LEVEL_DEBUG, message, ##__VA_ARGS__)
#define LOG_WARNING(message, ...) LOG(cLog, Logger::LogLevel::LOG_LEVEL_WARNING, message, ##__VA_ARGS__); LOG(fLog, Logger::LogLevel::LOG_LEVEL_WARNING, message, ##__VA_ARGS__)
#define LOG_ERROR(message, ...) LOG(cLog, Logger::LogLevel::LOG_LEVEL_ERROR, message, ##__VA_ARGS__); LOG(fLog, Logger::LogLevel::LOG_LEVEL_ERROR, message, ##__VA_ARGS__)
#define LOG_CRITICAL(message, ...) LOG(cLog, Logger::LogLevel::LOG_LEVEL_CRITICAL, message, ##__VA_ARGS__); LOG(fLog, Logger::LogLevel::LOG_LEVEL_CRITICAL, message, ##__VA_ARGS__)

// S(PECIFIC) Log, allows to call log on a specific Logger object, may be useful for Daily loggers
#define SLOG(logger, level, message, ...) (logger).Log(message, level, __FILE__, __LINE__, ##__VA_ARGS__)
#define SLOG_INFO(logger, message, ...) LOG(logger, Logger::LogLevel::LOG_LEVEL_INFO, message, ##__VA_ARGS__)
#define SLOG_OK(logger, message, ...) LOG(logger, Logger::LogLevel::LOG_LEVEL_OKSTATUS, message, ##__VA_ARGS__)
#define SLOG_DEBUG(logger, message, ...) LOG(logger, Logger::LogLevel::LOG_LEVEL_DEBUG, message, ##__VA_ARGS__)
#define SLOG_WARNING(logger, message, ...) LOG(logger, Logger::LogLevel::LOG_LEVEL_WARNING, message, ##__VA_ARGS__)
#define SLOG_ERROR(logger, message, ...) LOG(logger, Logger::LogLevel::LOG_LEVEL_ERROR, message, ##__VA_ARGS__)
#define SLOG_CRITICAL(logger, message, ...) LOG(logger, Logger::LogLevel::LOG_LEVEL_CRITICAL, message, ##__VA_ARGS__)
};

#endif
