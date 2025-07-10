#include "FileLogger.h"

#include <string>
#include <cstdarg>

FileLogger* FileLogger::Instance()
{
    static FileLogger instance;
    return &instance;
}

FileLogger::~FileLogger()
{
    if (logFile.is_open())
        logFile.close();
}

FileLogger::FileLogger()
{
    logFile.open(DEFAULT_LOG_FILE_NAME, std::ios::out | std::ios::app);
    if (!logFile.is_open())
        std::cerr << "Error while trying to open LogFile: " << DEFAULT_LOG_FILE_NAME << std::endl;
}

FileLogger::FileLogger(const std::string& filePath)
{
    logFile.open(filePath, std::ios::out | std::ios::app);
    if (!logFile.is_open())
        std::cerr << "Error while trying to open LogFile: " << DEFAULT_LOG_FILE_NAME << std::endl;
}


void FileLogger::Log(const std::string& message, Logger::LogLevel lvl, const char* file, int line, ...)
{
    // Prepare to handle variadic arguments
    va_list args;
    va_start(args, line); // 'line' is the last argument before variadic ones

    // Format the message (parse variadic arguments)
    std::string formattedMessage = FormatString(message.c_str(), args);

    std::lock_guard<std::mutex> guard(lMutex);

    if (!logFile.is_open())
    {
        std::cerr << "Attempt to log to an unopened file." << std::endl;
        return;
    }

    // Get timestamp
    std::string nowTimestamp = Utility::time_stamp();

    // Get level
    std::string levelStr = GetLogLevelStr(lvl);

    // Format and write the log message
    logFile << "[" << nowTimestamp << "] " << "[" << levelStr << "] ";

    if (file != nullptr)
        logFile << "[" << file << ":" << line << "] ";

    // Write the formatted message
    logFile << formattedMessage << std::endl;

    va_end(args);
}
