#ifndef TCP_SOCKET_MANAGER
#define TCP_SOCKET_MANAGER

#include "AuthSession.h"

#include "ConsoleLogger.h"
#include "FileLogger.h"

#include <mutex>

namespace NECRO
{
namespace Auth
{
	inline constexpr uint8_t	MANAGER_RESERVED_FDS = 2;
	inline constexpr uint16_t	MANAGER_SERVER_PORT = 61531;

	// The amout of time (in ms) that if passed will timeout the socket if TLS connection succeedes and no packet arrives
	inline constexpr uint32_t MANAGER_POST_TLS_IDLE_TIMEOUT_MS = 5000;
	inline constexpr uint32_t MANAGER_HANDSHAKING_IDLE_TIMEOUT_MS = 5000;

	inline constexpr uint32_t CONNECTION_ATTEMPT_CLEANUP_INTERVAL_MIN = 1;
	inline constexpr uint32_t MAX_CONNECTION_ATTEMPTS_PER_MINUTE = 10;

	//-----------------------------------------------------------------------------------------------------
	// Abstracts a TCP Socket Listener into a manager, that listens, accepts and manages connections
	//-----------------------------------------------------------------------------------------------------
	class TCPSocketManager
	{
	public:
		// Construct the socket manager
		TCPSocketManager(SocketAddressesFamily _family);

	protected:
		// Underlying listener socket
		TCPSocket m_listener;

		// Connections container
		std::vector<std::shared_ptr<AuthSession>> m_list;
		std::vector<std::shared_ptr<AuthSession>> m_toClose;

		// WakeUp socket loop
		TCPSocket					m_wakeListen;
		std::unique_ptr<TCPSocket>	m_wakeRead;
		std::mutex					m_writeMutex;
		std::atomic<bool>			m_writePending;
		TCPSocket					m_wakeWrite;

		// IP-based spam prevention <ip, last attempt>
		struct IPRequestData
		{
			std::chrono::steady_clock::time_point lastUpdate;
			size_t tries;
		};

		std::unordered_map<std::string, IPRequestData> m_ipRequestMap;

		void SetupWakeup();

	public:
		int Poll();

		void WakeUp();
	};

}
}

#endif
