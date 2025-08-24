#ifndef AUTH_SESSION_H
#define AUTH_SESSION_H

#include "TCPSocket.h"
#include <AuthCodes.h>
#include <unordered_map>
#include <array>
#include <chrono>

#include <mysqlx/xdevapi.h>

#include "AES.h"

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
    class AuthSession : public TCPSocket, public std::enable_shared_from_this<AuthSession>
    {
    private:
        AccountData m_data;
        bool        m_closeAfterSend; // when this is true, the SendCallback will close the socket. Used to close connection as soon as possible when a client is not valid


        std::chrono::steady_clock::time_point m_handshakeStartTime;
        std::chrono::steady_clock::time_point m_lastActivity;

    public:
        AuthSession(sock_t socket) : TCPSocket(socket), m_status(SocketStatus::GATHER_INFO), m_closeAfterSend(false)
        {
        }

        SocketStatus m_status;

        static std::unordered_map<uint8_t, AuthHandler> InitHandlers();

        AccountData& GetAccountData()
        {
            return m_data;
        }

        //----------------------------------------------------------------------------------------------------
        // Updates the internal time_point to now()
        //----------------------------------------------------------------------------------------------------
        void UpdateLastActivity()
        {
            m_lastActivity = std::chrono::steady_clock::now();
        }

        void SetHandshakeStartTime()
        {
            m_handshakeStartTime = std::chrono::steady_clock::now();
        }

        std::chrono::steady_clock::time_point GetLastActivity()
        {
            return m_lastActivity;
        }

        std::chrono::steady_clock::time_point GetHandshakeStartTime()
        {
            return m_handshakeStartTime;
        }

        int ReadCallback() override;
        void SendCallback() override;

        // Handlers functions
        bool HandleAuthLoginGatherInfoPacket();
        bool DBCallback_AuthLoginGatherInfoPacket(mysqlx::SqlResult& result);

        bool HandleAuthLoginProofPacket();
        bool DBCallback_AuthLoginProofPacket(mysqlx::SqlResult& result);
    };

}
}
#endif
