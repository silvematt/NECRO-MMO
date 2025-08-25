#include "TCPAcceptor.h"

namespace NECRO
{
	//---------------------------------------------------------------------------------------------------------------------------------------------
	// Sets the ptr to the socket that will receive the next accept.
	// It's used by the SocketManager to determine which socket receives the accept (so which ioContext the socket will be associated with
	//---------------------------------------------------------------------------------------------------------------------------------------------
	bool TCPAcceptor::SetInSocket(tcp::socket* ptr, uint32_t tID)
	{
		if (ptr)
		{
			m_inSocket = ptr;
			m_threadID = tID;

			return true;
		}
		else
		{
			LOG_ERROR("Error while setting m_inSocket. Given ptr was invalid!");
			m_inSocket = nullptr;
			tID = -1;

			return false;
		}
	}

	//---------------------------------------------------------------------------------------------------------------------------------------------
	// Binds the acceptor to 0.0.0.0
	//---------------------------------------------------------------------------------------------------------------------------------------------
	bool TCPAcceptor::Bind()
	{
		boost::system::error_code ec;
		m_acceptor.open(m_endpoint.protocol(), ec);
		m_acceptor.set_option(boost::asio::socket_base::reuse_address(true));

		if (ec)
		{
			LOG_ERROR("Error while opening Acceptor Socket.");
			return false;
		}

		m_acceptor.bind(m_endpoint, ec);

		if (ec)
		{
			LOG_ERROR("Error while binding Acceptor Socket.");
			return false;
		}

		m_acceptor.listen(NECRO_ACCEPTOR_LISTEN_MAX_CONN, ec);

		if (ec)
		{
			LOG_ERROR("Error while listening Acceptor Socket.");
			return false;
		}

		return true;
	}
}
