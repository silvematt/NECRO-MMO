#ifndef NECRO_TCP_ACCEPTOR_H
#define NECRO_TCP_ACCEPTOR_H

#include "FileLogger.h"
#include "ConsoleLogger.h"

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>

using boost::asio::ip::tcp;

namespace NECRO
{
	using boost::asio::ip::tcp;
	using boost::asio::ssl::stream;
	using boost::asio::ssl::context;

	inline constexpr uint32_t NECRO_ACCEPTOR_LISTEN_MAX_CONN = boost::asio::socket_base::max_listen_connections;

	//-----------------------------------------------------------------------------------------------------
	// Abstracts a boost:tcp::acceptor 
	//-----------------------------------------------------------------------------------------------------
	class TCPAcceptor
	{
		typedef stream<tcp::socket> ssl_socket;

	private:
		boost::asio::io_context& m_ioContextRef;

		tcp::acceptor	m_acceptor;
		tcp::endpoint	m_endpoint;

		// Accept side
		tcp::socket*	m_inSocket;	// Socket in which we'll pull in the connection, will refer to the respective io_Context of the NetworkThread that will manage this new conn
		int				m_threadID;

		bool m_Closed = false;
		bool m_Closing = false;

	public:
		TCPAcceptor(boost::asio::io_context& io, uint16_t port) :
			m_ioContextRef(io), m_endpoint(tcp::v4(), port),
			m_acceptor(m_ioContextRef), m_inSocket(nullptr), m_threadID(-1)
		{
		}

		// Sets the ptr to the socket that will receive the next accept.
		// It's used by the SocketManager to determine which socket receives the accept (so which ioContext the socket will be associated with
		bool SetInSocket(tcp::socket* ptr, uint32_t tID);

		// Binds the acceptor to 0.0.0.0
		bool Bind();

		//----------------------------------------------------------------------------------------------------------------------------------------------------------
		// Templated Accept, passing the SocketManager instance and the acceptCallback will allow us to define a callback that runs in the SocketManager instance.
		// That callback will manage the new connection
		//----------------------------------------------------------------------------------------------------------------------------------------------------------
		template <typename T, void (T::* acceptCallback)(tcp::socket&&, int)>
		void AsyncAccept(T* instance)
		{
			m_acceptor.async_accept(*m_inSocket, [this, instance](boost::system::error_code ec)
			{
				if (!ec)
				{
					(instance->*acceptCallback)(std::move(*m_inSocket), m_threadID);
				}
				else
				{
					LOG_ERROR("Error while accepting client socket.");
				}
			}
			);
		}
	};
}

#endif
