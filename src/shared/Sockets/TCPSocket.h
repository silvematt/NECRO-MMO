#ifndef TCP_SOCKET_H
#define TCP_SOCEKT_H

#ifdef _WIN32
	#include "WinSock2.h"
	#include <WS2tcpip.h>
#endif

#include <memory>
#include <cstdint>
#include <queue>

#include <openssl/ssl.h>

#include "SocketAddress.h"
#include "NetworkMessage.h"

#include "SocketUtility.h"
#include "ConsoleLogger.h"
#include "FileLogger.h"

namespace NECRO
{
#ifdef _WIN32
	typedef SOCKET sock_t;
#else
	typedef int sock_t;
#endif

	#define READ_BLOCK_SIZE 4096

	const int TCP_LISTEN_DEFUALT_BACKLOG = SOMAXCONN;

	enum class SocketAddressesFamily
	{
		INET = AF_INET,
		INET6 = AF_INET6
	};

	//-------------------------------------------------------
	// Defines a TCP Socket object.
	//-------------------------------------------------------
	class TCPSocket
	{
	protected:
		friend class SocketAddress;
		sock_t m_socket;

		SocketAddress remoteAddress;
		uint16_t remotePort;

		// Read/Write buffers
		NetworkMessage inBuffer;
		std::queue<NetworkMessage> outQueue;

		bool closed = false;

		// OpenSSL support
		bool usesTLS = false;
		SSL* ssl;
		BIO* bio;

		// Used for dynamic POLLIN | POLLOUT behavior, save the pfd and we'll update the events to look for in base of the content of the outQueue
		// If the outqueue is empty, only poll for POLLIN events, otherwise, also POLLOUT events
		// This is better than the callback Send() approach because even if a Send fails because the socket was not writable at the time of the callback, we'll still try to send the packets later
		pollfd* pfd;


	public:
		TCPSocket(SocketAddressesFamily family);
		TCPSocket(sock_t inSocket);

		~TCPSocket();

		virtual void OnConnectedCallback() {};
		virtual void ReadCallback() {};
		virtual void SendCallback() {};

		bool						IsOpen();

		int							Bind(const SocketAddress& addr);
		int							Listen(int backlog = TCP_LISTEN_DEFUALT_BACKLOG);

		// Templated Accept
		template<typename T = TCPSocket>
		std::shared_ptr<T> Accept(SocketAddress& addr)
		{
			static_assert(std::is_base_of<TCPSocket, T>::value || std::is_same<TCPSocket, T>::value, "T must be TCPSocket or derived from it");

			int addrLen = addr.GetSize();
			sock_t inSocket = accept(m_socket, &addr.m_addr, &addrLen);

			if (inSocket != INVALID_SOCKET)
			{
				std::shared_ptr<T> newSocket = std::make_shared<T>(inSocket);
				newSocket->remoteAddress = addr;
				newSocket->remotePort = ntohs(reinterpret_cast<sockaddr_in*>(&addr.m_addr)->sin_port);
				return newSocket;
			}
			else
			{
				if (!SocketUtility::ErrorIsWouldBlock())
				{
					LOG_ERROR(std::string("Error during TCPSocket::Accept()"));
				}
				return nullptr;
			}
		}

		int							Connect(const SocketAddress& addr);

		void						QueuePacket(NetworkMessage&& pckt);
		int							Send();
		int							Receive();

		std::string GetRemoteAddressAndPort()
		{
			return remoteAddress.RemoteAddressAndPortToString();
		}

		std::string GetRemoteAddress()
		{
			return remoteAddress.RemoteAddressToString();
		}

		void SetRemoteAddressAndPort(const SocketAddress& s, const uint16_t& p)
		{
			remoteAddress = s;
			remotePort = p;
		}

		void SetPfd(pollfd* fd)
		{
			pfd = fd;
		}

		bool HasPendingData() const
		{
			return !outQueue.empty();
		}

		int							Shutdown();
		int							Close();

		sock_t						GetSocketFD() { return m_socket; };

		int							SetBlockingEnabled(bool blocking);
		int							SetSocketOption(int lvl, int optName, const char* optVal, int optLen);

		SSL* GetSSL() { return ssl; }

		// OpenSSL
		void ServerTLSSetup(const char* hostname);
		void ClientTLSSetup(const char* hostname);
		int TLSPerformHandshake();
	};

}
#endif
