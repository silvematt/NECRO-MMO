#ifndef NECRO_NETWORK_THREAD_H
#define NECRO_NETWORK_THREAD_H

#include <thread>
#include <memory>
#include <vector>

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>

#include "FileLogger.h"
#include "ConsoleLogger.h"

#include <iostream>
#include <string>
#include <mutex>

namespace NECRO
{
	inline constexpr uint32_t NETWORK_THREAD_UPDATE_WAIT_MILLISEC_VAL = 1;

	using boost::asio::ip::tcp;
	using boost::asio::ssl::stream;
	using boost::asio::ssl::context;


	template<class SocketType>
	class NetworkThread
	{
		typedef stream<tcp::socket> ssl_socket;

	private:
		int											m_threadID;
		std::unique_ptr<std::thread>				m_thread;
		std::atomic<bool>							m_stopped;

		boost::asio::io_context						m_ioContext;
		boost::asio::ssl::context					m_sslContext;

		boost::asio::steady_timer					m_updateTimer;

		std::vector<std::shared_ptr<SocketType>>	m_sockets;
		std::atomic<size_t>							m_socketsNumber; // tracks both m_sockets and m_queuedSockets size

		std::vector<std::shared_ptr<SocketType>>	m_queuedSockets; // sockets queued up for insertion in the main m_sockets list
		std::mutex									m_queuedSocketsMutex;

		boost::asio::ip::tcp::socket				m_acceptSocket;		// the acceptor (that runs on the main thread's context) can choose to use this to pull in the new connection

	public:
		NetworkThread(int id, bool server) : 
			m_threadID(id), 
			m_thread(nullptr), 
			m_stopped(false), 
			m_ioContext(1), 
			m_updateTimer(m_ioContext), 
			m_sslContext(server ? boost::asio::ssl::context::tlsv13_server
								: boost::asio::ssl::context::sslv23_client),
			m_acceptSocket(m_ioContext)
		{
			if (server)
			{
				// Server: load certificate + private key
				m_sslContext.use_certificate_chain_file("server.pem");
				m_sslContext.use_private_key_file("pkey.pem", boost::asio::ssl::context::pem);

				// Force TLS 1.3 minimum if available
				SSL_CTX_set_min_proto_version(m_sslContext.native_handle(), TLS1_3_VERSION);

				// Set SSL options
				long opts = SSL_OP_IGNORE_UNEXPECTED_EOF | SSL_OP_NO_RENEGOTIATION | SSL_OP_CIPHER_SERVER_PREFERENCE;
				SSL_CTX_set_options(m_sslContext.native_handle(), opts);

				// Session cache
				SSL_CTX_set_session_cache_mode(m_sslContext.native_handle(), SSL_SESS_CACHE_SERVER);
				SSL_CTX_sess_set_cache_size(m_sslContext.native_handle(), 1024);
				SSL_CTX_set_timeout(m_sslContext.native_handle(), 3600);

				// Do not require client certificates
				m_sslContext.set_verify_mode(boost::asio::ssl::verify_none);

				// ECDH key generation for modern TLS
				SSL_CTX_set_ecdh_auto(m_sslContext.native_handle(), 1);
			}
			else
			{
				m_sslContext.set_verify_mode(boost::asio::ssl::verify_peer);
				m_sslContext.load_verify_file("server.pem");
			}
		}

	private:
		void Run()
		{
			// Start the thread
			m_updateTimer.expires_after(std::chrono::milliseconds(NETWORK_THREAD_UPDATE_WAIT_MILLISEC_VAL));
			m_updateTimer.async_wait([this](boost::system::error_code const& ec) { Update(); });

			m_ioContext.run();

			// Here the context went out of work (thread was stopped)
			m_sockets.clear();
			m_queuedSockets.clear();
		}

		void Update()
		{
			if (m_stopped)
				return;

			m_updateTimer.expires_after(std::chrono::milliseconds(NETWORK_THREAD_UPDATE_WAIT_MILLISEC_VAL));
			m_updateTimer.async_wait([this](boost::system::error_code const& ec) { Update(); });

			// Insert pending sockets
			AddQueuedSocketsToMainList();

			// Update the sockets
			int before = m_sockets.size();
			m_sockets.erase(std::remove_if(m_sockets.begin(), m_sockets.end(),
							[](auto& s) { return s->Update() == -1; }),
							m_sockets.end());
			m_socketsNumber.fetch_sub(before - m_sockets.size());
		}

		void AddQueuedSocketsToMainList()
		{
			std::lock_guard guard(m_queuedSocketsMutex);

			for (auto& s : m_queuedSockets)
				m_sockets.push_back(s);

			m_queuedSockets.clear();
		}

	public:
		int Start()
		{
			if (m_thread)
				return -1;

			m_thread = std::make_unique<std::thread>(&NetworkThread::Run, this);

			return 0;
		}

		size_t GetSocketsSize() const
		{
			return m_socketsNumber.load();
		}

		// Queues an already-created socket that will be inserted in the Network's Thread sockets list
		void QueueNewSocket(std::shared_ptr<SocketType> newSock)
		{
			std::lock_guard guard(m_queuedSocketsMutex);
			m_queuedSockets.push_back(newSock);
			m_socketsNumber++;
		}

		void Stop()
		{
			m_stopped = true;
			m_ioContext.stop();
		}

		void Join()
		{
			if (m_thread)
				m_thread->join();
		}

		boost::asio::io_context& GetIOContext()
		{
			return m_ioContext;
		}

		boost::asio::ssl::context& GetSSLContext()
		{
			return m_sslContext;
		}

		tcp::socket& GetAcceptSocket()
		{
			return m_acceptSocket;
		}

		tcp::socket* GetAcceptSocketPtr()
		{
			return &m_acceptSocket;
		}
	};
}

#endif
