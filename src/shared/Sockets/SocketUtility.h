#ifndef SOCKET_UTILITY_H
#define SOCKET_UTILITY_H

#ifdef _WIN32
#include "WinSock2.h"
#else
#include <sys/socket.h>
#endif

#include "ConsoleLogger.h"
#include "FileLogger.h"

constexpr int SOCKET_UTILITY_NO_ERROR = 0;

class SocketUtility
{
public:

	//---------------------------------------------------------
	// Returns the last error that occurred
	//---------------------------------------------------------
	static int GetLastError()
	{
#ifdef _WIN32
		return WSAGetLastError();
#else
		return errno;
#endif
	}

	static bool ErrorIsWouldBlock()
	{
#ifdef _WIN32
		if (GetLastError() == WSAEWOULDBLOCK)
			return true;
#else
		if (GetLastError() == EWOULDBLOCK || GetLastError() == EAGAIN) // EAGAIN is often the same as EWOULDBLOCK
			return true;
#endif

		return false;
	}

	static bool ErrorIsIsInProgres()
	{
#ifdef _WIN32
		if (GetLastError() == WSAEINPROGRESS)
			return true;
#else
		if (GetLastError() == EINPROGRESS)
			return true;
#endif

		return false;
	}

	static bool ErrorIsIsConnectionRefused()
	{
#ifdef _WIN32
		if (GetLastError() == WSAECONNREFUSED)
			return true;
#else
		if (GetLastError() == ECONNREFUSED)
			return true;
#endif

		return false;
	}

	//---------------------------------------------------------
	// Initializes Winsock if on windows, otherwise do nothing.
	//---------------------------------------------------------
	static void Initialize()
	{
#ifdef _WIN32
		WORD wVersion = MAKEWORD(2, 2);
		WSADATA wsaData;

		int startupValue = WSAStartup(wVersion, &wsaData);

		if (startupValue != 0)
		{
			LOG_ERROR(std::string("Error during SocketUtility::Initialize() [" + std::to_string(startupValue) + "]"));
			return;
		}

		LOG_OK("SocketUtility::Initialize() successful!");
#endif
	}
};

#endif
