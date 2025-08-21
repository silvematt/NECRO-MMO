#ifndef NECRO_CONSOLE_LOGGER_H
#define NECRO_CONSOLE_LOGGER_H

#include "Logger.h"

namespace NECRO
{

	class ConsoleLogger : public Logger
	{
	public:
		static ConsoleLogger& Instance()
		{
			static ConsoleLogger instance;
			return instance;
		}

		std::string GetColor(LogLevel lvl);

		virtual void Log(const std::string& message, Logger::LogLevel lvl, const char* file, int line, ...) override;
	};

}

#endif
