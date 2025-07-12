#include "TCPSocket.h"
#include "SocketUtility.h"

#ifndef _WIN32
#include <sys/socket.h>
#endif

#include <iostream>
#include <string>

#include "ConsoleLogger.h"
#include "FileLogger.h"
#include "OpenSSLManager.h"

namespace NECRO
{
	TCPSocket::TCPSocket(SocketAddressesFamily family)
	{
		ssl = nullptr;
		bio = nullptr;

		usesTLS = false;
		closed = false;

		m_socket = socket(static_cast<int>(family), SOCK_STREAM, IPPROTO_TCP);

		if (m_socket == INVALID_SOCKET)
		{
			LOG_ERROR(std::string("Error 1 during TCPSocket::Create()"));
			LOG_ERROR(std::to_string(SocketUtility::GetLastError()));
		}
	}

	TCPSocket::TCPSocket(sock_t inSocket)
	{
		ssl = nullptr;
		bio = nullptr;

		usesTLS = false;
		closed = false;

		m_socket = inSocket;

		if (m_socket == INVALID_SOCKET)
		{
			LOG_ERROR(std::string("Error 2 during TCPSocket::Create()"));
			LOG_ERROR(std::to_string(SocketUtility::GetLastError()));
		}
	}

	TCPSocket::~TCPSocket()
	{
		Close();
	}

	bool TCPSocket::IsOpen()
	{
		return (!closed);
	}

	int TCPSocket::Bind(const SocketAddress& addr)
	{
		int err = bind(m_socket, &addr.m_addr, addr.GetSize());

		if (err != 0)
		{
			LOG_ERROR(std::string("Error during TCPSocket::Bind() [" + std::to_string(SocketUtility::GetLastError()) + "]"));
			return SocketUtility::GetLastError();
		}

		return SOCKET_UTILITY_NO_ERROR;
	}

	int TCPSocket::Listen(int backlog)
	{
		int err = listen(m_socket, backlog);

		if (err != 0)
		{
			LOG_ERROR(std::string("Error during TCPSocket::Listen() [" + std::to_string(SocketUtility::GetLastError()) + "]"));
			return SocketUtility::GetLastError();
		}

		return SOCKET_UTILITY_NO_ERROR;
	}

	int TCPSocket::Connect(const SocketAddress& addr)
	{
		int err = connect(m_socket, &addr.m_addr, addr.GetSize());

		if (err != 0)
		{
			if (!SocketUtility::ErrorIsWouldBlock() && !SocketUtility::ErrorIsIsInProgres())
			{
				LOG_ERROR(std::string("Error during TCPSocket::Connect() [") + std::to_string(SocketUtility::GetLastError()) + "]");
				return SocketUtility::GetLastError();
			}
		}

		return SOCKET_UTILITY_NO_ERROR;
	}

	void TCPSocket::QueuePacket(NetworkMessage&& pckt)
	{
		outQueue.push(std::move(pckt));

		// Update pfd events
		if (pfd)
			pfd->events = POLLIN | (HasPendingData() ? POLLOUT : 0);
	}

	int TCPSocket::Send()
	{
		if (outQueue.empty())
			return 0;

		NetworkMessage& out = outQueue.front();

		int bytesSent = 0;
		size_t sslBytesSent;
		if (!usesTLS)
		{
			bytesSent = send(m_socket, reinterpret_cast<const char*>(out.GetReadPointer()), out.GetActiveSize(), 0);

			if (bytesSent < 0)
			{
				if (SocketUtility::ErrorIsWouldBlock())
					return 0;

				Shutdown();

				LOG_ERROR(std::string("Error during TCPSocket::Send() [") + std::to_string(SocketUtility::GetLastError()) + "]");
				return SocketUtility::GetLastError();
			}
		}
		else
		{
			int ret = SSL_write_ex(ssl, reinterpret_cast<const char*>(out.GetReadPointer()), out.GetActiveSize(), &sslBytesSent);

			if (ret <= 0)
			{
				int sslError = SSL_get_error(ssl, ret);
				if (sslError == SSL_ERROR_WANT_READ || sslError == SSL_ERROR_WANT_WRITE)
					return 0;

				Shutdown();
				LOG_ERROR(std::string("Error during TCPSocket::Send() [") +
					std::to_string(sslError) + "]");
				return sslError;
			}
		}

		if (usesTLS)
			bytesSent = sslBytesSent;

		// Mark that 'bytesSent' were sent
		out.ReadCompleted(bytesSent);

		if (out.GetActiveSize() == 0)
			outQueue.pop(); // if whole packet was sent, pop it from the queue, otherwise we had a short send and will come back later

		// SendCallback(); needed?

		// Update pfd events
		if (pfd)
			pfd->events = POLLIN | (HasPendingData() ? POLLOUT : 0);

		return bytesSent;
	}

	int TCPSocket::Receive()
	{
		if (!IsOpen())
			return 0;

		inBuffer.CompactData();
		inBuffer.EnlargeBufferIfNeeded();

		// Manually write on the inBuffer
		int bytesReceived = 0;
		size_t sslBytesReceived = 0;
		if (!usesTLS)
		{
			bytesReceived = recv(m_socket, reinterpret_cast<char*>(inBuffer.GetWritePointer()), inBuffer.GetRemainingSpace(), 0);

			if (bytesReceived < 0)
			{
				if (SocketUtility::ErrorIsWouldBlock())
					return 0;

				Shutdown();
				LOG_ERROR(std::string("Error during TCPSocket::Receive() [") + std::to_string(SocketUtility::GetLastError()) + "]");
				return -1;
			}
		}
		else
		{
			int ret = SSL_read_ex(ssl, reinterpret_cast<char*>(inBuffer.GetWritePointer()), inBuffer.GetRemainingSpace(), &sslBytesReceived);

			if (ret <= 0)
			{
				int sslError = SSL_get_error(ssl, ret);
				if (sslError == SSL_ERROR_WANT_READ || sslError == SSL_ERROR_WANT_WRITE)
					return 0;
				else if (sslError == SSL_ERROR_ZERO_RETURN)
				{
					// Shutdown gracefully
					LOG_DEBUG("Received SSL_ERROR_ZERO_RETURN. Shutting down socket gracefully.");
					Shutdown();
					return 0;
				}
				else
				{
					Shutdown();
					LOG_ERROR(std::string("Error during TCPSocket::Receive() [") + std::to_string(SocketUtility::GetLastError()) + "]");
					return sslError;
				}
			}
		}

		if (usesTLS)
			bytesReceived = sslBytesReceived;

		// Make sure to update the write pos
		inBuffer.WriteCompleted(bytesReceived);

		LOG_INFO("Received %d bytes of something!", bytesReceived);

		ReadCallback();	// this will handle the data we've received

		return bytesReceived;
	}

	int TCPSocket::SetBlockingEnabled(bool blocking)
	{
#ifdef _WIN32
		u_long mode = blocking ? 0 : 1;
		int result = ioctlsocket(m_socket, FIONBIO, &mode);

		if (result != 0)
		{
			LOG_ERROR(std::string("Error during TCPSocket::SetBlockingEnabled(" + std::to_string(blocking) + ") [") + std::to_string(SocketUtility::GetLastError()) + "]");
			return SocketUtility::GetLastError();
		}

		return SOCKET_UTILITY_NO_ERROR;
#else
		int flags = fcntl(fd, F_GETFL, 0);

		if (flags == -1)
		{
			LOG_ERROR(std::string("Error during TCPSocket::SetBlockingEnabled(" + std::to_string(blocking) + ") [") + std::to_string(SocketUtility::GetLastError()) + "]");
			return SocketUtility::GetLastError();
		}

		flags = blocking ? (flags & ~O_NONBLOCK) : (flags | O_NONBLOCK);

		result = fcntl(fd, F_SETFL, flags);

		if (result != 0)
		{
			LOG_ERROR(std::string("Error during TCPSocket::SetBlockingEnabled(" + std::to_string(blocking) + ") [") + std::to_string(SocketUtility::GetLastError()) + "]");
			return SocketUtility::GetLastError();
		}

		return SOCKET_UTILITY_NO_ERROR;
#endif
	}

	int TCPSocket::SetSocketOption(int lvl, int optName, const char* optVal, int optLen)
	{
		int optResult = setsockopt(m_socket, lvl, optName, optVal, optLen);

		if (optResult != 0)
		{
			LOG_ERROR(std::string("Error during TCPSocket::SetSocketOption(" + std::to_string(optName) + ") [") + std::to_string(SocketUtility::GetLastError()) + "]");
			return SocketUtility::GetLastError();
		}

		return SOCKET_UTILITY_NO_ERROR;
	}

	int TCPSocket::Shutdown()
	{
		if (closed)
			return 0;

		closed = true;

		// Free OpenSSL data
		if (ssl != nullptr)
		{
			SSL_free(ssl);
			ssl = nullptr;
		}
#ifdef _WIN32
		int result = shutdown(m_socket, SD_SEND);

		if (result < 0)
			LOG_ERROR("Error while shutting down the socket");

		return result;
#else
		int result = shutdown(m_socket, SHUT_WR);

		if (result < 0)
			LOG_ERROR("Error while shutting down the socket");

		return result;
#endif
	}

	int TCPSocket::Close()
	{
		// Free OpenSSL data
		if (ssl != nullptr)
		{
			SSL_free(ssl);
			ssl = nullptr;
		}
#ifdef _WIN32

		int result = closesocket(m_socket);
		return result;
#else
		int result = close(m_socket);
		return result;
#endif
	}

	// OpenSSL
	void TCPSocket::ServerTLSSetup(const char* hostname)
	{
		usesTLS = true;

		bio = OpenSSLManager::CreateBioAndWrapSocket(GetSocketFD());
		ssl = OpenSSLManager::ServerCreateSSLObject(bio);

		OpenSSLManager::SetSNIHostname(ssl, hostname);
		OpenSSLManager::SetCertVerificationHostname(ssl, hostname);
	}

	void TCPSocket::ClientTLSSetup(const char* hostname)
	{
		usesTLS = true;

		bio = OpenSSLManager::CreateBioAndWrapSocket(GetSocketFD());
		ssl = OpenSSLManager::ClientCreateSSLObject(bio);

		OpenSSLManager::SetSNIHostname(ssl, hostname);
		OpenSSLManager::SetCertVerificationHostname(ssl, hostname);
	}

	int TCPSocket::TLSPerformHandshake()
	{
		int ret;
		bool success = true;
		// Perform the handshake
		while ((ret = SSL_connect(ssl)) != 1)
		{
			int err = SSL_get_error(ssl, ret);

			// Keep trying
			if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE)
				continue;

			if (err == SSL_ERROR_WANT_CONNECT || err == SSL_ERROR_WANT_ACCEPT)
				continue;

			if (err == SSL_ERROR_ZERO_RETURN)
			{
				LOG_INFO("TLS connection closed by peer during handshake.");
				success = false;
				break;
			}

			if (err == SSL_ERROR_SYSCALL)
			{
				LOG_ERROR("System call error during TLS handshake. Ret: %d.", ret);
				success = false;
				break;
			}

			// Otherwise, we got an error
			LOG_ERROR("TLSPerformHandshake failed!");
			if (err == SSL_ERROR_SSL)
			{
				if (SSL_get_verify_result(ssl) != X509_V_OK)
					LOG_ERROR("Verify error: %s\n", X509_verify_cert_error_string(SSL_get_verify_result(ssl)));

				success = false;
			}
			break;
		}

		// Handshake performed!
		return (success) ? 1 : 0;
	}

}
