#ifndef AUTH_SESSION_H
#define AUTH_SESSION_H

#include "TCPSocket.h"
#include <AuthCodes.h>

#include <unordered_map>

namespace NECRO
{
namespace Client
{
    class AuthSession;

    #pragma pack(push, 1)
    struct AuthHandler
    {
        NECRO::Auth::SocketStatus status;
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
        AuthSession(SocketAddressesFamily fam) : TCPSocket(fam), status(NECRO::Auth::SocketStatus::GATHER_INFO) {}
        AuthSession(sock_t socket) : TCPSocket(socket), status(NECRO::Auth::SocketStatus::GATHER_INFO) {}
        NECRO::Auth::SocketStatus status;

        static std::unordered_map<uint8_t, AuthHandler> InitHandlers();

        void OnConnectedCallback() override;
        void ReadCallback() override;

        // Handlers functions
        bool HandlePacketAuthLoginGatherInfoResponse();
        bool HandlePacketAuthLoginProofResponse();
    };

}
}

#endif
