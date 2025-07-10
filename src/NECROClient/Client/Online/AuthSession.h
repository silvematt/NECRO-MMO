#ifndef AUTH_SESSION_H
#define AUTH_SESSION_H

#include "TCPSocket.h"
#include <unordered_map>

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

//----------------------------------------------------------------------------------------------------
// AuthSession is the extension of the base TCPSocket class, that defines the methods and
// functionality that defines the exchange of messages with the connected server
//----------------------------------------------------------------------------------------------------
class AuthSession : public TCPSocket
{
public:
    AuthSession(SocketAddressesFamily fam) : TCPSocket(fam), status(STATUS_GATHER_INFO) {}
    AuthSession(sock_t socket) : TCPSocket(socket), status(STATUS_GATHER_INFO) {}
    AuthStatus status;

    static std::unordered_map<uint8_t, AuthHandler> InitHandlers();

    void OnConnectedCallback() override;
    void ReadCallback() override;

    // Handlers functions
    bool HandlePacketAuthLoginGatherInfoResponse();
    bool HandlePacketAuthLoginProofResponse();
};

#endif
