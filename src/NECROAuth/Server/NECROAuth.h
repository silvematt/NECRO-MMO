#ifndef NECROAUTH_H
#define NECROAUTH_H

#include "ConsoleLogger.h"
#include "FileLogger.h"
#include "TCPSocketManager.h"

#include "LoginDatabase.h"
#include "DatabaseWorker.h"

constexpr uint8_t CLIENT_VERSION_MAJOR = 1;
constexpr uint8_t CLIENT_VERSION_MINOR = 0;
constexpr uint8_t CLIENT_VERSION_REVISION = 0;

class NECROAuth
{
public:
	NECROAuth() :
		isRunning(false)
	{

	}

private:
	// Status
	bool isRunning;

	ConsoleLogger cLogger;
	FileLogger fLogger;
	std::unique_ptr<TCPSocketManager> sockManager;

	// directdb will be used for queries that run (and block) on the main thread
	// to use only with data-critical code, for example, while making a response packet where an information in the database is needed to go further
	// for "fire-and-forget" operations like updating logs, fields, etc we can use the dbworker
	LoginDatabase directdb; 
	DatabaseWorker dbworker;

public:
	ConsoleLogger& GetConsoleLogger();
	FileLogger& GetFileLogger();
	TCPSocketManager& GetSocketManager();

	LoginDatabase& GetDirectDB();
	DatabaseWorker& GetDBWorker();

	int						Init();
	void					Start();
	void					Update();
	void					Stop();
	int						Shutdown();
};

// Global access for the Server 
extern NECROAuth server;

// Inline functions
inline ConsoleLogger& NECROAuth::GetConsoleLogger()
{
	return cLogger;
}

inline FileLogger& NECROAuth::GetFileLogger()
{
	return fLogger;
}

inline TCPSocketManager& NECROAuth::GetSocketManager()
{
	return *sockManager.get();
}

inline LoginDatabase& NECROAuth::GetDirectDB()
{
	return directdb;
}

inline DatabaseWorker& NECROAuth::GetDBWorker()
{
	return dbworker;
}

#endif
