#include "FileLogger.h"

#include <string>
#include <cstdarg>

namespace NECRO
{
    FileLogger* FileLogger::Instance()
    {
        static FileLogger instance;
        return &instance;
    }

    FileLogger::~FileLogger()
    {
        if (m_logFile.is_open())
            m_logFile.close();
    }

    FileLogger::FileLogger()
    {
        m_logFile.open(DEFAULT_LOG_FILE_NAME, std::ios::out | std::ios::app);
        if (!m_logFile.is_open())
            std::cerr << "Error while trying to open LogFile: " << DEFAULT_LOG_FILE_NAME << std::endl;
    }

    FileLogger::FileLogger(const std::string& filePath)
    {
        m_logFile.open(filePath, std::ios::out | std::ios::app);
        if (!m_logFile.is_open())
            std::cerr << "Error while trying to open LogFile: " << DEFAULT_LOG_FILE_NAME << std::endl;
    }


    void FileLogger::Log(const std::string& message, Logger::LogLevel lvl, const char* file, int line, ...)
    {
        // Prepare to handle variadic arguments
        va_list args;
        va_start(args, line); // 'line' is the last argument before variadic ones

        // Format the message (parse variadic arguments) (not used anymore since FMT)
        // std::string formattedMessage = FormatString(message.c_str(), args);

        std::lock_guard<std::mutex> guard(m_logMutex);

        if (!m_logFile.is_open())
        {
            std::cerr << "Attempt to log to an unopened file." << std::endl;
            return;
        }

        // Get timestamp
        std::string nowTimestamp = Utility::time_stamp();

        // Get level
        std::string levelStr = GetLogLevelStr(lvl);

        // Format and write the log message
        m_logFile << "[" << nowTimestamp << "] " << "[" << levelStr << "] ";

        if (file != nullptr)
            m_logFile << "[" << file << ":" << line << "] ";

        // Write the formatted message
        m_logFile << message << std::endl;

        va_end(args);
    }

}
