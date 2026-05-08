#ifndef WORLD_SESSION_H
#define WORLD_SESSION_H

#include "TCPSocket.h"
#include <WorldCodes.h>

#include <unordered_map>

namespace NECRO
{
namespace Client
{
    class WorldSession;

    #pragma pack(push, 1)
    struct WorldHandler
    {
        NECRO::World::WorldSocketStatus status;
        size_t packetSize;
        bool (WorldSession::* handler)();
    };
    #pragma pack(pop)

    //----------------------------------------------------------------------------------------------------
    // WorldSession is the client-side extension of TCPSocket that defines the methods and
    // functionality that defines the exchange of messages with the connected World Server
    //----------------------------------------------------------------------------------------------------
    class WorldSession : public TCPSocket
    {
    public:
        WorldSession(SocketAddressesFamily fam) : TCPSocket(fam), m_status(NECRO::World::WorldSocketStatus::GATHER_SESSIONKEY) {}
        WorldSession(sock_t socket) : TCPSocket(socket), m_status(NECRO::World::WorldSocketStatus::GATHER_SESSIONKEY) {}

        NECRO::World::WorldSocketStatus m_status;

        static std::unordered_map<uint8_t, WorldHandler> InitHandlers();

        void    OnConnectedCallback() override;
        int     ReadCallback() override;
        void    SendCallback() override;
    };

}
}

#endif
