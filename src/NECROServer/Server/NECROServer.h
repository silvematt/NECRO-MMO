#ifndef NECROSERVER_H
#define NECROSERVER_H

#include "ConsoleLogger.h"
#include "FileLogger.h"

class NECROServer
{
private:
	// Status
	bool isRunning;

	ConsoleLogger cLogger;
	FileLogger fLogger;

public:
	ConsoleLogger& GetConsoleLogger();
	FileLogger& GetFileLogger();

	int						Init();
	void					Update();
	void					Stop();
	int						Shutdown();
};

// Global access for the Server 
extern NECROServer server;

// Inline functions
inline ConsoleLogger& NECROServer::GetConsoleLogger()
{
	return cLogger;
}

inline FileLogger& NECROServer::GetFileLogger()
{
	return fLogger;
}

#endif
