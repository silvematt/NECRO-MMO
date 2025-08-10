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

private:
	ssl_socket m_socket;

public:
	HammerSocket(boost::asio::io_context& io, context& ssl_ctx) : m_socket(io, ssl_ctx)
	{
		try
		{
			tcp::resolver resolver(io);
			auto endpoints = resolver.resolve("192.168.1.221", "61531");

			boost::asio::connect(m_socket.lowest_layer(), endpoints);
			m_socket.lowest_layer().set_option(tcp::no_delay(true));

			m_socket.set_verify_mode(boost::asio::ssl::verify_peer);
			//m_socket.set_verify_callback(boost::asio::ssl::host_name_verification("host.name"));

			int randAction = rand() % 3;

			if (randAction == 1)
			{
				m_socket.handshake(boost::asio::ssl::stream<tcp::socket>::client);
			}
			else if (randAction == 2)
			{
				char x[100];

				int randAction2 = rand() % 3;
				x[0] = randAction;

				m_socket.write_some(boost::asio::buffer(x));
			}
		}
		catch (...)
		{

		}
		// Send some trash
		//char x[100];
		//x[0] = 0;
		//m_socket.write_some(boost::asio::buffer(x));
	}
};

#endif
