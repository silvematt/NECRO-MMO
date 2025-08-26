#ifndef BOOST_TCP_SOCKET_H
#define BOOST_TCP_SOCKET_H

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>

#include "NetworkMessage.h"
#include "ConsoleLogger.h"
#include "FileLogger.h"

#include <queue>
#include <chrono>

#include "inerithable_shared_from_this.h"

using boost::asio::ip::tcp;
using boost::asio::ssl::stream;
using boost::asio::ssl::context;

namespace NECRO
{
	//-------------------------------------------------------
	// Defines a TCP Socket object that uses boost.
	//-------------------------------------------------------
	class TCPSocketBoost : public inheritable_enable_shared_from_this<TCPSocketBoost>
	{
		using inheritable_enable_shared_from_this<TCPSocketBoost>::shared_from_this;

	protected:
		typedef stream<tcp::socket> ssl_socket;

		enum class UnderlyingState
		{
			DEFAULT = 0,
			RESOLVING,
			CONNECTING,
			HANDSHAKING,
			JUST_CONNECTED, // this is used in the Update() method to start an operation (like the first AsyncRead and first packet send) as soon as the connection is established
			CONNECTED,
			CRITICAL_ERROR	// this is used to signal the manager of this socket (like the NetworkThread) to get rid of this socket
		};

	protected:
		boost::asio::io_context& m_ioContextRef;

		tcp::socket m_socket;

		// TLS
		bool m_usesTLS = false;
		std::unique_ptr<ssl_socket> m_sslSocket;

		UnderlyingState m_UnderlyingState;

		// Read/Write buffers
		NetworkMessage				m_inBuffer;
		std::queue<NetworkMessage>	m_outQueue;

		bool m_AsyncWriting = false;
		bool m_ShutDown = false;
		bool m_Closed = false;
		bool m_Closing = false;


		void SetupTLS(context& sslCtx);

		// Client connection
		void Resolve(const std::string& ip, const std::string& port);
		void OnResolve(tcp::resolver::results_type results);
		void OnConnected();

		void AsyncRead();
		void AsyncWrite();

		void QueuePacket(NetworkMessage&& pckt);


		void			InternalReadCallback(boost::system::error_code err, std::size_t transferredBytes);
		void			InternalWriteCallback(boost::system::error_code err, std::size_t writtenBytes);

		virtual int		AsyncReadCallback() = 0;
		virtual void	AsyncWriteCallback() = 0;

		void			CloseSocket();

	public:
		// Constructs a plain socket
		TCPSocketBoost(boost::asio::io_context& io) : m_usesTLS(false), m_ioContextRef(io), 
						m_UnderlyingState(UnderlyingState::DEFAULT), m_socket(m_ioContextRef)
		{
		}

		// Constructs a TLS socket
		TCPSocketBoost(boost::asio::io_context& io, context& sslCtx) : m_usesTLS(true), m_ioContextRef(io), 
						m_UnderlyingState(UnderlyingState::DEFAULT), m_socket(m_ioContextRef)
		{
			SetupTLS(sslCtx);
		}

		// Construct a plain socket moving in
		TCPSocketBoost(tcp::socket&& insock) : m_usesTLS(false), m_ioContextRef(static_cast<boost::asio::io_context&>(insock.get_executor().context())), 
												m_UnderlyingState(UnderlyingState::DEFAULT), m_socket(std::move(insock))
		{
		}

		// Constructs a TLS socket moving in
		TCPSocketBoost(tcp::socket&& insock, context& sslCtx) : m_usesTLS(true), m_ioContextRef(static_cast<boost::asio::io_context&>(insock.get_executor().context())),
			m_UnderlyingState(UnderlyingState::DEFAULT), m_socket(std::move(insock))
		{
			SetupTLS(sslCtx);
		}

		virtual int Update(std::chrono::steady_clock::time_point now) { return 0; };

		bool IsOpen()
		{
			return !m_Closed && !m_Closing;
		}

		std::string GetRemoteAddressAndPort()
		{
			try 
			{
				auto endpoint = m_sslSocket->lowest_layer().remote_endpoint();
				return endpoint.address().to_string() + ":" + std::to_string(endpoint.port());
			}
			catch (const boost::system::system_error& e) 
			{
				// handle error, e.g., socket not connected
				return "Error: " + std::string(e.what());
			}
		}

		std::string GetRemoteAddress()
		{
			try
			{
				if (m_usesTLS)
				{
					auto endpoint = m_sslSocket->lowest_layer().remote_endpoint();
					return endpoint.address().to_string();
				}
				else
				{
					auto endpoint = m_socket.remote_endpoint();
					return endpoint.address().to_string();
				}
			}
			catch (const boost::system::system_error& e)
			{
				// handle error, e.g., socket not connected
				return "Error: " + std::string(e.what());
			}
		}
	};
}

#endif
