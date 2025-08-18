#include "TCPSocketBoost.h"

namespace NECRO
{
	void TCPSocketBoost::SetupTLS(context& sslCtx)
	{
		m_sslSocket = std::make_unique<ssl_socket>(std::move(m_socket), sslCtx);
	}

	void TCPSocketBoost::Resolve(const std::string& ip, const std::string& port)
	{
		m_state = State::RESOLVING;

		// Resolve
		// Use shared_ptr to keep resolver alive
		auto resolver = std::make_shared<tcp::resolver>(m_ioContextRef);
		auto self = shared_from_this();
		resolver->async_resolve(ip, port, [this, self, resolver](const boost::system::error_code& ec, tcp::resolver::results_type results)
			{
				if (!ec)
					OnResolve(results);
				else
					m_state = State::CRITICAL_ERROR;
			}
		);
	}

	void TCPSocketBoost::OnResolve(tcp::resolver::results_type results)
	{
		m_state = State::CONNECTING;

		if (m_usesTLS)
		{
			auto self = shared_from_this();
			boost::asio::async_connect(m_sslSocket->lowest_layer(), results.begin(), results.end(),
				[this, self](const boost::system::error_code& ec, tcp::resolver::results_type::iterator i)
				{
					if (!ec)
					{
						// Connected, need to handshake
						m_sslSocket->lowest_layer().set_option(tcp::no_delay(true));
						m_sslSocket->set_verify_mode(boost::asio::ssl::verify_peer);

						OnConnected();
					}
					else
						m_state = State::CRITICAL_ERROR;
				});
		}
		else
		{
			auto self = shared_from_this();
			boost::asio::async_connect(m_socket, results.begin(), results.end(),
				[this, self](const boost::system::error_code& ec, tcp::resolver::results_type::iterator i)
				{
					if (!ec)
					{
						// Connected, need to handshake
						m_socket.set_option(tcp::no_delay(true));

						OnConnected();
					}
					else
						m_state = State::CRITICAL_ERROR;
				});
		}
	}

	void TCPSocketBoost::OnConnected()
	{
		if (m_usesTLS)
		{
			m_state = State::HANDSHAKING;

			auto self = shared_from_this();
			m_sslSocket->async_handshake(boost::asio::ssl::stream_base::client,
				[this, self](const boost::system::error_code& ec)
				{
					if (!ec)
					{
						m_state = State::JUST_CONNECTED;

						// Now we can start reading/writing in Update()
					}
					else
						m_state = State::CRITICAL_ERROR;
				});
		}
		else
			m_state = State::JUST_CONNECTED;
	}

	void TCPSocketBoost::AsyncRead()
	{
		if (!IsOpen())
			return;

		m_inBuffer.CompactData();
		m_inBuffer.EnlargeBufferIfNeeded();

		if (m_usesTLS)
		{
			m_sslSocket->async_read_some(boost::asio::buffer(m_inBuffer.GetWritePointer(), m_inBuffer.GetRemainingSpace()), 
											std::bind(&TCPSocketBoost::InternalReadCallback, this->shared_from_this(), std::placeholders::_1, std::placeholders::_2));
		}
		else
		{
			m_socket.async_read_some(boost::asio::buffer(m_inBuffer.GetWritePointer(), m_inBuffer.GetRemainingSpace()),
				std::bind(&TCPSocketBoost::InternalReadCallback, this->shared_from_this(), std::placeholders::_1, std::placeholders::_2));
		}
	}

	// This ensures that even if AsyncReadCallback is overridden, m_inBuffer.WriteCompleted will always be called
	void TCPSocketBoost::InternalReadCallback(boost::system::error_code err, size_t transferredBytes)
	{
		if (err)
		{
			LOG_ERROR("ERROR ON InternalReadCallback {}. SHUTTING DOWN SOCKET!", err.what());
			CloseSocket();
		}
		else
		{
			m_inBuffer.WriteCompleted(transferredBytes);
			AsyncReadCallback();
		}
	}

	void TCPSocketBoost::QueuePacket(NetworkMessage&& pckt)
	{
		m_outQueue.push(std::move(pckt));

		// Make sure queue is processed
		AsyncWrite();
	}

	// AsyncWrites do work without syncronization issues because at most one thread at the time calls these handlers
	void TCPSocketBoost::AsyncWrite()
	{
		if (m_AsyncWriting)
			return;

		m_AsyncWriting = true;

		// Process a message
		NetworkMessage& out = m_outQueue.front();

		if (m_usesTLS)
		{
			m_sslSocket->async_write_some(boost::asio::buffer(out.GetReadPointer(), out.GetActiveSize()), 
											std::bind(&TCPSocketBoost::InternalWriteCallback, this->shared_from_this(), std::placeholders::_1, std::placeholders::_2));
		}
		else
		{
			m_socket.async_write_some(boost::asio::buffer(out.GetReadPointer(), out.GetActiveSize()),
				std::bind(&TCPSocketBoost::InternalWriteCallback, this->shared_from_this(), std::placeholders::_1, std::placeholders::_2));
		}
	}

	void TCPSocketBoost::InternalWriteCallback(boost::system::error_code err, std::size_t writtenBytes)
	{
		if (err)
		{
			LOG_ERROR("ERROR ON InternalWriteCallback {}. SHUTTING DOWN SOCKET!", err.what());
			CloseSocket();
		}
		else
		{
			m_AsyncWriting = false;

			m_outQueue.front().ReadCompleted(writtenBytes);

			// Check if the whole message was sent
			if (m_outQueue.front().GetActiveSize() <= 0)
				m_outQueue.pop();

			// Check if there are still things in the outqueue
			if (m_outQueue.size() > 0)
				AsyncWrite();

			AsyncWriteCallback();
		}
	}

	void TCPSocketBoost::CloseSocket()
	{
		if (m_Closing)
			return;

		m_Closing = true;

		if (m_usesTLS)
		{
			auto self = shared_from_this();
			m_sslSocket->async_shutdown([this, self](const boost::system::error_code& ec)
			{
				if (!ec)
				{
					// TLS shutdown completed
				}
				else if (ec == boost::asio::error::eof)
				{
					// Remote closed cleanly
				}
				else
				{
					// TLS shutdown failed
				}

				boost::system::error_code close_ec;
				m_sslSocket->lowest_layer().close(close_ec);
				m_Closed = true;
				m_state = State::CRITICAL_ERROR; // this will allow the manager to clean up this socket
			});
		}
		else
		{
			boost::system::error_code close_ec;
			m_socket.close(close_ec);
			m_Closed = true;
			m_state = State::CRITICAL_ERROR; // this will allow the manager to clean up this socket
		}
	}
}
