#include "ConsoleLogger.h"
#include <cstdarg>

ConsoleLogger* ConsoleLogger::Instance()
{
    static ConsoleLogger instance;
    return &instance;
}

std::string ConsoleLogger::GetColor(Logger::LogLevel lvl)
{
    switch (lvl)
    {
    case LogLevel::LOG_LEVEL_INFO:
        return "\033[37m"; // White

    case LogLevel::LOG_LEVEL_DEBUG:
        return "\033[36m"; // Cyan

    case LogLevel::LOG_LEVEL_OKSTATUS:
        return "\033[32m"; // Green

    case LogLevel::LOG_LEVEL_WARNING:
        return "\033[33m"; // Yellow

    case LogLevel::LOG_LEVEL_ERROR:
        return "\033[31m"; // Red

    case LogLevel::LOG_LEVEL_CRITICAL:
        return "\033[35m"; // Purple

    default:
        return "\033[0m"; // Reset
    }
}

void ConsoleLogger::Log(const std::string& message, Logger::LogLevel lvl, const char* file, int line, ...)
{
    // Prepare to handle variadic arguments
    va_list args;
    va_start(args, line); // 'line' is the last argument before variadic ones

    // Format the message (parse variadic arguments)
    std::string formattedMessage = FormatString(message.c_str(), args);

    // lock_guard will automatically release the mutex as soon as it is destroyed
    std::lock_guard<std::mutex> guard(lMutex);

    // Get timestamp
    std::string nowTimestamp = Utility::time_stamp();

    // Get color
    std::string colorCode = GetColor(lvl);

    // Get level
    std::string lvlStr = GetLogLevelStr(lvl);

    std::cout << colorCode << "[" << nowTimestamp << "] " << "[" << lvlStr << "]";

    if (file != nullptr)
        std::cout << " [" << file << ":" << line << "]";

    // Print the formatted message
    std::cout << " " << formattedMessage << "\033[0m" << std::endl;

    va_end(args);
}
