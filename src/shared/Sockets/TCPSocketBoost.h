#ifndef BOOST_TCP_SOCKET_H
#define BOOST_TCP_SOCKET_H

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>

#include "NetworkMessage.h"
#include "ConsoleLogger.h"
#include "FileLogger.h"

#include <queue>

using boost::asio::ip::tcp;
using boost::asio::ssl::stream;
using boost::asio::ssl::context;

namespace NECRO
{
	//-------------------------------------------------------
	// Defines a TCP Socket object that uses boost.
	//-------------------------------------------------------
	class TCPSocketBoost : public std::enable_shared_from_this<TCPSocketBoost>
	{
	protected:
		typedef stream<tcp::socket> ssl_socket;

		enum class State
		{
			DEFAULT = 0,
			RESOLVING,
			CONNECTING,
			HANDSHAKING,
			JUST_CONNECTED, // this is used in the Update() method to start an operation (like the first AsyncRead and first packet send) as soon as the connection is established
			CONNECTED,
			CRITICAL_ERROR
		};

	protected:
		boost::asio::io_context& m_ioContextRef;

		tcp::socket m_socket;

		// TLS
		bool m_usesTLS = false;
		std::unique_ptr<ssl_socket> m_sslSocket;

		State m_state;

		// Read/Write buffers
		NetworkMessage				m_inBuffer;
		std::queue<NetworkMessage>	m_outQueue;

		virtual int Update() { return 0; };

		void SetupTLS(context& sslCtx);

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

		bool m_AsyncWriting = false;
		bool m_ShutDown = false;
		bool m_Closed = false;
		bool m_Closing = false;

	public:
		// Constructs a plain socket
		TCPSocketBoost(boost::asio::io_context& io) : m_usesTLS(false), m_ioContextRef(io), m_state(State::DEFAULT), m_socket(m_ioContextRef)
		{
			
		}

		// Constructs a TLS socket
		TCPSocketBoost(boost::asio::io_context& io, context& sslCtx) : m_usesTLS(true), m_ioContextRef(io), m_state(State::DEFAULT), m_socket(m_ioContextRef)
		{
			SetupTLS(sslCtx);
		}

		bool IsOpen()
		{
			return !m_Closed && !m_Closing;
		}
	};
}

#endif
