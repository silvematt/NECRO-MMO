#ifndef AUTH_SESSION_H
#define AUTH_SESSION_H

#include "TCPSocket.h"
#include <unordered_map>
#include <array>

#include "AES.h"

// Status of the current socket
enum AuthStatus
{
    STATUS_GATHER_INFO = 0,
    STATUS_LOGIN_ATTEMPT,
    STATUS_AUTHED,
    STATUS_CLOSED
};

class AuthSession;
#pragma pack(push, 1)

struct AuthHandler
{
    AuthStatus status;
    size_t packetSize;
    bool (AuthSession::* handler)();
};

#pragma pack(pop)

struct AccountData
{
    std::string username;
    uint32_t accountID; // accountid in the database

    std::array<uint8_t, AES_128_KEY_SIZE> sessionKey;
    NECROAES::IV iv;
};

//----------------------------------------------------------------------------------------------------
// AuthSession is the extension of the base TCPSocket class, that defines the methods and
// functionality that defines the exchange of messages with the connected client on the other end
//----------------------------------------------------------------------------------------------------
class AuthSession : public TCPSocket
{
private:
    AccountData data;

public:
    AuthSession(sock_t socket) : TCPSocket(socket), status(STATUS_GATHER_INFO) {}
    AuthStatus status;

    static std::unordered_map<uint8_t, AuthHandler> InitHandlers();

    AccountData& GetAccountData()
    {
        return data;
    }

    void ReadCallback() override;

    // Handlers functions
    bool HandleAuthLoginGatherInfoPacket();
    bool HandleAuthLoginProofPacket();

};

#endif
