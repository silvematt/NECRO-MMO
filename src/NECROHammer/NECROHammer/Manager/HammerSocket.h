#ifndef NECRO_HAMMER_SOCKET_H
#define NECRO_HAMMER_SOCKET_H

#include "TCPSocketBoost.h"
#include "AuthCodes.h"

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>

using boost::asio::ip::tcp;
using boost::asio::ssl::stream;
using boost::asio::ssl::context;

namespace NECRO
{
namespace Hammer
{
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
	};

	class HammerSocket : public TCPSocketBoost
	{
    protected:
		std::string m_remoteIp;
		std::string m_remotePort;

        Auth::SocketStatus m_status;
		AuthData m_data;

	public:
		// TLS-enabled constructor
		HammerSocket(boost::asio::io_context& io, context& ssl_ctx, std::string ip, std::string port) : TCPSocketBoost(io, ssl_ctx), m_status(Auth::SocketStatus::GATHER_INFO), m_remoteIp(ip), m_remotePort(port)
		{
            m_data.username = "matt";
            m_data.password = "124";
		}

		// Or non-TLS version
		HammerSocket(boost::asio::io_context& io, std::string ip, std::string port) : TCPSocketBoost(io), m_status(Auth::SocketStatus::GATHER_INFO), m_remoteIp(ip), m_remotePort(port)
		{
            m_data.username = "matt";
            m_data.password = "124";
		}

        static std::unordered_map<uint8_t, HammerHandler> InitHandlers();

		// This runs in the NetworkThread that possess this socket
		int Update() override;

		int AsyncReadCallback() override;
		void AsyncWriteCallback() override;

        bool HandlePacketAuthLoginGatherInfoResponse();
        bool HandlePacketAuthLoginProofResponse();
	};
}
}

#endif
