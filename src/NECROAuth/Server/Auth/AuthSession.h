#ifndef AUTH_SESSION_H
#define AUTH_SESSION_H

#include "TCPSocketBoost.h"
#include <AuthCodes.h>
#include <unordered_map>
#include <array>
#include <chrono>

#include <mysqlx/xdevapi.h>

#include "AES.h"
#include "inerithable_shared_from_this.h"

namespace NECRO
{
namespace Auth
{
    class AuthSession;
    #pragma pack(push, 1)

    struct AuthHandler
    {
        SocketStatus status;
        size_t packetSize;
        bool (AuthSession::* handler)();
    };

    #pragma pack(pop)

    struct AccountData
    {
        std::string username;
        uint32_t accountID; // accountid in the database

        std::array<uint8_t, AES_128_KEY_SIZE> sessionKey;
        AES::IV iv;

        uint8_t versionMajor;
        uint8_t versionMinor;
        uint8_t versionRevision;

        std::string pass;
        uint32_t randIVPrefix;
    };

    //----------------------------------------------------------------------------------------------------
    // AuthSession is the extension of the base TCPSocket class, that defines the methods and
    // functionality that defines the exchange of messages with the connected client on the other end
    //----------------------------------------------------------------------------------------------------
    class AuthSession : public TCPSocketBoost, public inheritable_enable_shared_from_this<AuthSession>
    {
        using inheritable_enable_shared_from_this<AuthSession>::shared_from_this;    
    
    private:
        AccountData m_data;
        bool        m_closeAfterSend; // when this is true, the SendCallback will close the socket. Used to close connection as soon as possible when a client is not valid


        std::chrono::steady_clock::time_point   m_handshakeStartTime;
        boost::asio::steady_timer               m_handshakeTimeoutTimer;
        bool                                    m_handshakeTimedout;

        std::chrono::steady_clock::time_point   m_lastActivity;

    public:
        AuthSession(boost::asio::io_context& io, context& ssl_ctx) : TCPSocketBoost(io, ssl_ctx), m_status(SocketStatus::GATHER_INFO), m_closeAfterSend(false), m_handshakeTimeoutTimer(m_ioContextRef), m_handshakeTimedout(false)
        {
        }

        AuthSession(tcp::socket&& insocket, context& ssl_ctx) : TCPSocketBoost(std::move(insocket), ssl_ctx), m_status(SocketStatus::GATHER_INFO), m_closeAfterSend(false), m_handshakeTimeoutTimer(m_ioContextRef), m_handshakeTimedout(false)
        {
        }


        SocketStatus m_status;

        static std::unordered_map<uint8_t, AuthHandler> InitHandlers();

        void HandshakeTimeoutHandler(boost::system::error_code const& ec);

        // This runs in the NetworkThread that possess this socket
        int     Update(std::chrono::steady_clock::time_point now) override;

        int     AsyncReadCallback() override;
        void    AsyncWriteCallback() override;

        // Handlers functions
        bool HandleAuthLoginGatherInfoPacket();
        bool DBCallback_AuthLoginGatherInfoPacket(mysqlx::SqlResult& result);

        bool HandleAuthLoginProofPacket();
        bool DBCallback_AuthLoginProofPacket(mysqlx::SqlResult& result);

        AccountData& GetAccountData()
        {
            return m_data;
        }
    };

}
}

#endif
