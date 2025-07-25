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
		shutdownState = TLSShutdownState::PHASE_1;
		m_ssl = nullptr;
		m_bio = nullptr;

		m_usesTLS = false;
		m_ShutDown = false;

		m_socket = socket(static_cast<int>(family), SOCK_STREAM, IPPROTO_TCP);

		if (m_socket == INVALID_SOCKET)
		{
			LOG_ERROR(std::string("Error 1 during TCPSocket::Create()"));
			LOG_ERROR(std::to_string(SocketUtility::GetLastError()));
		}
	}

	TCPSocket::TCPSocket(sock_t inSocket)
	{
		shutdownState = TLSShutdownState::PHASE_1;
		m_ssl = nullptr;
		m_bio = nullptr;

		m_usesTLS = false;
		m_ShutDown = false;

		m_socket = inSocket;

		if (m_socket == INVALID_SOCKET)
		{
			LOG_ERROR(std::string("Error 2 during TCPSocket::Create()"));
			LOG_ERROR(std::to_string(SocketUtility::GetLastError()));
		}
	}


	bool TCPSocket::IsShutDown()
	{
		return m_ShutDown;
	}

	bool TCPSocket::IsOpen()
	{
		return !m_Closed;
	}

	int TCPSocket::Bind(const SocketAddress& addr)
	{
		int err = bind(m_socket, &addr.m_addr, addr.GetSize());

		if (err != 0)
		{
			LOG_ERROR(std::string("Error during TCPSocket::Bind() [" + std::to_string(SocketUtility::GetLastError()) + "]"));
			return SocketUtility::GetLastError();
		}

		return SocketUtility::SU_NO_ERROR_VAL;
	}

	int TCPSocket::Listen(int backlog)
	{
		int err = listen(m_socket, backlog);

		if (err != 0)
		{
			LOG_ERROR(std::string("Error during TCPSocket::Listen() [" + std::to_string(SocketUtility::GetLastError()) + "]"));
			return SocketUtility::GetLastError();
		}

		return SocketUtility::SU_NO_ERROR_VAL;
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

		return SocketUtility::SU_NO_ERROR_VAL;
	}

	void TCPSocket::QueuePacket(NetworkMessage&& pckt)
	{
		m_outQueue.push(std::move(pckt));

		// Update pfd events
		if (m_pfd)
			m_pfd->events = POLLIN | (HasPendingData() ? POLLOUT : 0);
	}

	int TCPSocket::Send()
	{
		if (m_outQueue.empty())
			return 0;

		NetworkMessage& out = m_outQueue.front();

		int bytesSent = 0;
		size_t sslBytesSent;
		if (!m_usesTLS)
		{
			bytesSent = send(m_socket, reinterpret_cast<const char*>(out.GetReadPointer()), out.GetActiveSize(), 0);

			if (bytesSent < 0)
			{
				if (SocketUtility::ErrorIsWouldBlock())
					return 0;

				LOG_ERROR(std::string("Error during TCPSocket::Send() [") + std::to_string(SocketUtility::GetLastError()) + "]");
				return -1;
			}
		}
		else
		{
			int ret = SSL_write_ex(m_ssl, reinterpret_cast<const char*>(out.GetReadPointer()), out.GetActiveSize(), &sslBytesSent);

			if (ret <= 0)
			{
				int sslError = SSL_get_error(m_ssl, ret);
				if (sslError == SSL_ERROR_WANT_READ || sslError == SSL_ERROR_WANT_WRITE)
					return 0;

				LOG_ERROR(std::string("Error during TCPSocket::Send() [") + std::to_string(sslError) + "]");
				return -1;
			}
		}

		if (m_usesTLS)
			bytesSent = sslBytesSent;

		// Mark that 'bytesSent' were sent
		out.ReadCompleted(bytesSent);

		if (out.GetActiveSize() == 0)
			m_outQueue.pop(); // if whole packet was sent, pop it from the queue, otherwise we had a short send and will come back later

		// SendCallback(); needed?

		// Update pfd events
		if (m_pfd)
			m_pfd->events = POLLIN | (HasPendingData() ? POLLOUT : 0);

		return bytesSent;
	}

	int TCPSocket::SysSend(const char* buf, int len)
	{
		int bytesSent = 0;
		bytesSent = send(m_socket, buf, len, 0);

		if (bytesSent < 0)
		{
			if (SocketUtility::ErrorIsWouldBlock())
				return 0;

			//Shutdown();
			Close();

			LOG_ERROR(std::string("Error during TCPSocket::SysSend() [") + std::to_string(SocketUtility::GetLastError()) + "]");
			return -1;
		}

		return bytesSent;
	}

	int TCPSocket::Receive()
	{
		if (!IsOpen())
			return 0;

		m_inBuffer.CompactData();
		m_inBuffer.EnlargeBufferIfNeeded();

		// Manually write on the inBuffer
		int bytesReceived = 0;
		size_t sslBytesReceived = 0;
		if (!m_usesTLS)
		{
			bytesReceived = recv(m_socket, reinterpret_cast<char*>(m_inBuffer.GetWritePointer()), m_inBuffer.GetRemainingSpace(), 0);

			if (bytesReceived < 0)
			{
				if (SocketUtility::ErrorIsWouldBlock())
					return 0;

				LOG_ERROR(std::string("Error during TCPSocket::Receive() [") + std::to_string(SocketUtility::GetLastError()) + "]");
				return -1;
			}
		}
		else
		{
			int ret = SSL_read_ex(m_ssl, reinterpret_cast<char*>(m_inBuffer.GetWritePointer()), m_inBuffer.GetRemainingSpace(), &sslBytesReceived);

			if (ret <= 0)
			{
				int sslError = SSL_get_error(m_ssl, ret);

				if (sslError == SSL_ERROR_WANT_READ || sslError == SSL_ERROR_WANT_WRITE)
					return 0;
				else if (sslError == SSL_ERROR_ZERO_RETURN)
				{
					// Shutdown gracefully
					LOG_DEBUG("Received SSL_ERROR_ZERO_RETURN. Shutting down socket gracefully.");
					return -1;
				}
				else
				{
					LOG_ERROR(std::string("Error during TCPSocket::Receive() [") + std::to_string(SocketUtility::GetLastError()) + "]");
					return -1;
				}
			}
		}

		if (m_usesTLS)
			bytesReceived = sslBytesReceived;

		// Make sure to update the write pos
		m_inBuffer.WriteCompleted(bytesReceived);

		LOG_INFO("Received {} bytes of something!", bytesReceived);

		if(ReadCallback() == -1)	// this will handle the data we've received, unless it returns -1 (error)
			return -1;

		return bytesReceived;
	}

	int TCPSocket::SysReceive(char* buf, int len)
	{
		int bytesReceived = 0;
		bytesReceived = recv(m_socket, buf, len, 0);

		if (bytesReceived < 0)
		{
			if (SocketUtility::ErrorIsWouldBlock())
				return 0;

			//Shutdown();
			Close();

			LOG_ERROR(std::string("Error during TCPSocket::Receive() [") + std::to_string(SocketUtility::GetLastError()) + "]");
			return -1;
		}

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

		return SocketUtility::SU_NO_ERROR_VAL;
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

		return SocketUtility::SU_NO_ERROR_VAL;
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

		return SocketUtility::SU_NO_ERROR_VAL;
	}

	int TCPSocket::Shutdown()
	{
		if (m_ShutDown) 
			return 0;

		m_ShutDown = true;

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
		if (m_Closed)
			return 0;

		m_Closed = true;

		int result = 0;

		try
		{
			// Free OpenSSL data, SSL_free also frees BIO
			if (m_ssl != nullptr)
			{
				SSL_free(m_ssl);
				m_ssl = nullptr;
			}
		}
		catch(...)
		{
			LOG_CRITICAL("Caught exception while trying to free OpenSSL data.");
		}

#ifdef _WIN32
		result = closesocket(m_socket);
#else
		result = close(m_socket);
#endif

		return result;
	}

	// OpenSSL
	void TCPSocket::ServerTLSSetup(const char* hostname)
	{
		m_usesTLS = true;

		m_bio = OpenSSLManager::CreateBioAndWrapSocket(GetSocketFD());
		m_ssl = OpenSSLManager::ServerCreateSSLObject(m_bio);

		OpenSSLManager::SetSNIHostname(m_ssl, hostname);
		OpenSSLManager::SetCertVerificationHostname(m_ssl, hostname);
	}

	void TCPSocket::ClientTLSSetup(const char* hostname)
	{
		m_usesTLS = true;

		m_bio = OpenSSLManager::CreateBioAndWrapSocket(GetSocketFD());
		m_ssl = OpenSSLManager::ClientCreateSSLObject(m_bio);

		OpenSSLManager::SetSNIHostname(m_ssl, hostname);
		OpenSSLManager::SetCertVerificationHostname(m_ssl, hostname);
	}

	int TCPSocket::TLSPerformHandshake()
	{
		int ret;
		bool success = true;
		// Perform the handshake
		while ((ret = SSL_connect(m_ssl)) != 1)
		{
			int err = SSL_get_error(m_ssl, ret);

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
				LOG_ERROR("System call error during TLS handshake. Ret: {}.", ret);
				success = false;
				break;
			}

			// Otherwise, we got an error
			LOG_ERROR("TLSPerformHandshake failed!");
			if (err == SSL_ERROR_SSL)
			{
				if (SSL_get_verify_result(m_ssl) != X509_V_OK)
					LOG_ERROR("Verify error: {}\n", X509_verify_cert_error_string(SSL_get_verify_result(m_ssl)));

				success = false;
			}
			break;
		}

		// Handshake performed!
		return (success) ? 1 : 0;
	}

	// Performs a full TLS Shutdown
	int TCPSocket::TLSShutdown() 
	{
		if (shutdownState == TLSShutdownState::DONE || !m_usesTLS)
			return 0;

		int ret = SSL_shutdown(m_ssl);

		if (ret == 1) 
		{
			// Got 1: full bidirectional shutdown complete
			shutdownState = TLSShutdownState::DONE;
			return 0; // completed
		}
		if (ret == 0) 
		{
			// We sent our close_notify but haven't seen theirs yet
			if (shutdownState == TLSShutdownState::PHASE_1)
			{
				shutdownState = TLSShutdownState::PHASE_2;
				return 1;
			}
			else 
			{
				// Already in phase2 and still got 0: try again later
				return 1;
			}
		}

		if (ret < 0) 
		{
			int err = SSL_get_error(m_ssl, ret);
			
			if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE) // wait for fd to be readable/writable then retry TLSShutdown()
				return 1;
		}

		// Should never get here unless we have an error (socket on the other end closed abruptly
		return -1;
	}

}
