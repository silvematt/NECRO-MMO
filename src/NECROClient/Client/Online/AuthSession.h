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
        NECRO::Auth::SocketStatus status = NECRO::Auth::SocketStatus::GATHER_INFO;
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
        AuthSession(SocketAddressesFamily fam) : TCPSocket(fam), m_status(NECRO::Auth::SocketStatus::GATHER_INFO) {}
        AuthSession(sock_t socket) : TCPSocket(socket), m_status(NECRO::Auth::SocketStatus::GATHER_INFO) {}
        NECRO::Auth::SocketStatus m_status;

        static std::unordered_map<uint8_t, AuthHandler> InitHandlers();

        void OnConnectedCallback() override;
        int ReadCallback() override;

        // Handlers functions
        bool HandlePacketAuthLoginGatherInfoResponse();
        bool HandlePacketAuthLoginProofResponse();
        bool HandlePacketGatherRealmsResponse();
    };

}
}

#endif
