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
	inline constexpr uint8_t SOCK_MANAGER_RESERVED_FDS = 2;

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

		// m_listener has index 0
		// m_WakeRead has index 1
		// Clients go from SOCK_MANAGER_RESERVED_FDS to n
		std::vector<pollfd> m_poll_fds;

		// WakeUp socket loop
		TCPSocket					m_wakeListen;
		std::unique_ptr<TCPSocket>	m_wakeRead;
		std::mutex					m_writeMutex;
		TCPSocket					m_wakeWrite;

		pollfd SetupWakeup();

	public:
		int Poll();

		void WakeUp();
	};

}
}

#endif
