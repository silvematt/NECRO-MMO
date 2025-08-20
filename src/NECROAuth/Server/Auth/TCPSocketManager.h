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
	// AuthServer
	inline constexpr uint8_t	MANAGER_RESERVED_FDS = 2;

	inline constexpr int		SERVER_POLL_TIMEOUT_MS = 30000;

	// The amout of time (in ms) that if passed will timeout the socket if TLS connection succeedes and no packet arrives
	inline constexpr uint32_t	MANAGER_POST_TLS_IDLE_TIMEOUT_MS = 10000;
	inline constexpr uint32_t	MANAGER_HANDSHAKING_IDLE_TIMEOUT_MS = 10000;

	// Database keep alive
	inline constexpr int		DB_KEEP_ALIVE_REQUEST_INTERVAL_MS = 60000; // 1 keep alive request every minute

	//-----------------------------------------------------------------------------------------------------
	// Abstracts a TCP Socket Listener into a manager, that listens, accepts and manages connections
	//-----------------------------------------------------------------------------------------------------
	class TCPSocketManager
	{
	public:
		// Settings that can be edited from the config file
		struct ConfigSettings
		{
			uint16_t	MANAGER_SERVER_PORT = 61531;
			
			// Spam prevention
			bool		ENABLE_SPAM_PREVENTION = 1;
			uint32_t	CONNECTION_ATTEMPT_CLEANUP_INTERVAL_MIN = 1;
			uint32_t	MAX_CONNECTION_ATTEMPTS_PER_MINUTE = 10;
		};

	public:
		// Construct the socket manager
		TCPSocketManager(SocketAddressesFamily _family);


	private:
		ConfigSettings m_configSettings;

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

		// DB Keep Alive
		std::chrono::steady_clock::time_point m_dbKeepAliveLastActivity;

		void ApplySettings();
		void SetupWakeup();

	public:
		int		Poll();
		void	WakeUp();

		const ConfigSettings& GetSettings() const
		{
			return m_configSettings;
		}
	};

}
}

#endif
