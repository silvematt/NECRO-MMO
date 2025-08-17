#ifndef NECRO_HAMMER_SOCKET_H
#define NECRO_HAMMER_SOCKET_H

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>

using boost::asio::ip::tcp;
using boost::asio::ssl::stream;
using boost::asio::ssl::context;

class HammerSocket
{
	typedef stream<tcp::socket> ssl_socket;

	enum class HSSocketState
	{
		DEFAULT = 0,
		RESOLVING,
		CONNECTING,
		HANDSHAKING,
		CONNECTED,
		CRITICAL_ERROR
	};

private:
	boost::asio::io_context& m_ioContextRef;
	ssl_socket m_socket;

	HSSocketState m_state;

public:
	HammerSocket(boost::asio::io_context& io, context& ssl_ctx) : m_ioContextRef(io), m_socket(m_ioContextRef, ssl_ctx), m_state(HSSocketState::DEFAULT)
	{
		
	}

	void Initialize()
	{
		m_state = HSSocketState::RESOLVING;

		// Resolve
		// Use shared_ptr to keep resolver alive
		auto resolver = std::make_shared<tcp::resolver>(m_ioContextRef);
		resolver->async_resolve("192.168.1.221", "61531", [this, resolver](const boost::system::error_code& ec, tcp::resolver::results_type results) 
			{
				if (!ec) 
					OnResolve(results);
				else 
					m_state = HSSocketState::CRITICAL_ERROR;
			}
		);
	}

	void OnResolve(tcp::resolver::results_type results)
	{
		m_state = HSSocketState::CONNECTING;

		boost::asio::async_connect(m_socket.lowest_layer(), results.begin(), results.end(),
			[this](const boost::system::error_code& ec, tcp::resolver::results_type::iterator i)
			{
				if (!ec)
				{
					// Connected, need to handshake
					m_socket.lowest_layer().set_option(tcp::no_delay(true));
					m_socket.set_verify_mode(boost::asio::ssl::verify_peer);

					OnConnected();
				}
				else
					m_state = HSSocketState::CRITICAL_ERROR;
			});
	}

	void OnConnected()
	{
		m_state = HSSocketState::HANDSHAKING;

		m_socket.async_handshake(boost::asio::ssl::stream_base::client,
			[this](const boost::system::error_code& ec)
			{
				if (!ec)
				{
					m_state = HSSocketState::CONNECTED;

					// Now we can start reading/writing in Update()
				}
				else
					m_state = HSSocketState::CRITICAL_ERROR;
			});
	}

	// This runs in the NetworkThread that possess this socket
	int Update()
	{
		// Signal this socket is dead and must be removed
		if (m_state == HSSocketState::CRITICAL_ERROR)
			return -1;

		if (m_state == HSSocketState::DEFAULT)
		{
			Initialize();
			return 0;
		}
		else if(m_state == HSSocketState::CONNECTED)
		{
			// Do stuff
		}

		return 0;
	}

};

#endif
