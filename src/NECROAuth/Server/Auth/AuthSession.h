#ifndef AUTH_SESSION_H
#define AUTH_SESSION_H

#include "TCPSocket.h"
#include <AuthCodes.h>
#include <unordered_map>
#include <array>

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
    };

    //----------------------------------------------------------------------------------------------------
    // AuthSession is the extension of the base TCPSocket class, that defines the methods and
    // functionality that defines the exchange of messages with the connected client on the other end
    //----------------------------------------------------------------------------------------------------
    class AuthSession : public TCPSocket
    {
    private:
        AccountData m_data;

    public:
        AuthSession(sock_t socket) : TCPSocket(socket), m_status(SocketStatus::GATHER_INFO) 
        {
        }

        SocketStatus m_status;

        static std::unordered_map<uint8_t, AuthHandler> InitHandlers();

        AccountData& GetAccountData()
        {
            return m_data;
        }

        int ReadCallback() override;

        // Handlers functions
        bool HandleAuthLoginGatherInfoPacket();
        bool HandleAuthLoginProofPacket();

    };

}
}
#endif
