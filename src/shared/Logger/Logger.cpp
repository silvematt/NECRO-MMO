#include "Logger.h"

std::string Logger::GetLogLevelStr(Logger::LogLevel lvl)
{
    switch (lvl)
    {
    case LogLevel::LOG_LEVEL_DEBUG:
        return "DEBUG";

    case LogLevel::LOG_LEVEL_INFO:
        return "INFO";

    case LogLevel::LOG_LEVEL_OKSTATUS:
        return "OK";

    case LogLevel::LOG_LEVEL_WARNING:
        return "WARNING";

    case LogLevel::LOG_LEVEL_ERROR:
        return "ERROR";

    case LogLevel::LOG_LEVEL_CRITICAL:
        return "CRITICAL";

    default:
        return "DEFAULT";
    }
}
