#pragma once

#include "TCPSocketBoost.h"
#include "AuthCodes.h"
#include "Realm.h"

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>

using boost::asio::ip::tcp;
using boost::asio::ssl::stream;
using boost::asio::ssl::context;

namespace NECRO
{
namespace Hammer
{
	// ------------------------------------------------------------------------------------------------
	//	These are all the different kind of behaviors the hammer sockets can have
	// ------------------------------------------------------------------------------------------------
	enum class HammerSocketType : uint32_t
	{
		SUCCESSFUL_AUTHENTICATION = 0,
		WRONG_PASSWORD,
		USERNAME_DOESNT_EXIST,
		FAIL_PROOF_OF_WORK,
		CONNECT_AND_HANG,
		CONNECT_AND_DONT_RESOLVE,
		//INIT_TLS_HANDSHAKE_AND_HANG,
		HANG_MID_AUTHENTICATION,
		LAST_VAL
	};

    class HammerSocket;
    #pragma pack(push, 1)

    struct HammerHandler
    {
        NECRO::Auth::SocketStatus status;
        size_t packetSize;
        bool (HammerSocket::* handler)();
    };

    #pragma pack(pop)

	struct AuthData
	{
		std::string username;
		std::string password;

		std::string ipAddress;

		std::array<uint8_t, AES_128_KEY_SIZE> sessionKey;
		std::array<uint8_t, AES_128_KEY_SIZE> greetcode;

		AES::IV iv;

		bool hasAuthenticated = false;
		std::vector<Realm> realmlist{};
	};

	class HammerSocket : public TCPSocketBoost, public inheritable_enable_shared_from_this<HammerSocket>
	{
		using inheritable_enable_shared_from_this<HammerSocket>::shared_from_this;

    protected:
		// To have different behaviors for hammer sockets
		HammerSocketType m_type;

		// Used to run some only once-code at the first Update() 
		bool m_init = false;

		std::string m_remoteIp;
		std::string m_remotePort;

        Auth::SocketStatus m_status;
		AuthData m_data;

		// Sockets of certain types (like CONNECT_AND_DONT_RESOLVE) will remain up indefinitely, this death timer, if set will kill the socket indiscriminately 
		bool m_deathCalled = false;
		int m_deathTimerMs = 0;
		boost::asio::steady_timer m_deathTimer;

	public:
		// TLS-enabled constructor
		HammerSocket(HammerSocketType t, boost::asio::io_context& io, context& ssl_ctx, std::string ip, std::string port) : m_type(t), TCPSocketBoost(io, ssl_ctx), m_status(Auth::SocketStatus::GATHER_INFO), m_remoteIp(ip), m_remotePort(port), m_deathTimer(io), m_init(false)
		{
			// Default
			m_data.username = "matt";
			m_data.password = "123";
			m_deathTimerMs = 0;

			if (m_type == HammerSocketType::SUCCESSFUL_AUTHENTICATION || m_type == HammerSocketType::HANG_MID_AUTHENTICATION)
			{
				m_data.username = "matt";
				m_data.password = "123";
			}
			else if (m_type == HammerSocketType::WRONG_PASSWORD)
			{
				m_data.username = "matt";
				m_data.password = "124";
			}
			else if (m_type == HammerSocketType::USERNAME_DOESNT_EXIST)
			{
				m_data.username = "invalid_username";
				m_data.password = "124";
			}
			else if (m_type == HammerSocketType::CONNECT_AND_HANG || m_type == HammerSocketType::CONNECT_AND_DONT_RESOLVE)
			{
				// Enables the death timer, these kind of socket will not cleanup by themselves as some of their functionalities will never run, including the socket's is-alive check.
				m_deathTimerMs = 15000;
			}
		}

		// Or non-TLS version
		HammerSocket(HammerSocketType t, boost::asio::io_context& io, std::string ip, std::string port) : TCPSocketBoost(io), m_type(t), m_status(Auth::SocketStatus::GATHER_INFO), m_remoteIp(ip), m_remotePort(port), m_deathTimer(io), m_init(false)
		{
			// Default
			m_data.username = "matt";
			m_data.password = "123";
			m_deathTimerMs = 0;

			if (m_type == HammerSocketType::SUCCESSFUL_AUTHENTICATION || m_type == HammerSocketType::HANG_MID_AUTHENTICATION)
			{
				m_data.username = "matt";
				m_data.password = "123";
			}
			else if (m_type == HammerSocketType::WRONG_PASSWORD)
			{
				m_data.username = "matt";
				m_data.password = "124";
			}
			else if (m_type == HammerSocketType::USERNAME_DOESNT_EXIST)
			{
				m_data.username = "invalid_username";
				m_data.password = "124";
			}
			else if (m_type == HammerSocketType::CONNECT_AND_HANG || m_type == HammerSocketType::CONNECT_AND_DONT_RESOLVE)
			{
				// Death timer here should always be set to be higher than the server's threshold
				m_deathTimerMs = 10000;
			}
		}

        static std::unordered_map<uint8_t, HammerHandler> InitHandlers();

		// This runs in the NetworkThread that possess this socket
		int Update(std::chrono::steady_clock::time_point now) override;

		int AsyncReadCallback() override;
		void AsyncWriteCallback() override;

        bool HandlePacketAuthLoginGatherInfoResponse();
        bool HandlePacketAuthLoginProofResponse();

		bool HandlePacketGatherRealmsResponse();
	};
}
}
