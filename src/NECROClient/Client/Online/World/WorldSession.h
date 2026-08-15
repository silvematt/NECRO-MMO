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
        size_t packetSize; // packet size is not needed if the worldpackets are encrypted via AES-GCM, as their size-check is done during decryption. However I kept this here if in the future I'd want to remove ecryption at all
        bool (WorldSession::* handler)();
    };
    #pragma pack(pop)

    //----------------------------------------------------------------------------------------------------
    // WorldSession is the client-side extension of TCPSocket that defines the methods and
    // functionality that defines the exchange of messages with the connected World Server
    //----------------------------------------------------------------------------------------------------
    class WorldSession : public TCPSocket
    {
    private:
        // When an encrypted packet arrives [size, iv, tag, ciphertext(innerpacket)], it gets decrypted and the innerpacket gets put into here
        // To read the innerpacket, the handlers use m_currentDecryptedPacket. The content of m_currentDecryptedPacket is valid only for the current handler's execution
        NetworkMessage m_currentDecryptedPacket;

        NECRO::World::WorldSocketStatus m_status;
        uint64_t m_expectedCounterForNextPacket = 0;

    public:
        WorldSession(SocketAddressesFamily fam) : TCPSocket(fam), m_status(NECRO::World::WorldSocketStatus::SELECTING_CHARACTERS) {}
        WorldSession(sock_t socket) : TCPSocket(socket), m_status(NECRO::World::WorldSocketStatus::SELECTING_CHARACTERS) {}

        static std::unordered_map<uint16_t, WorldHandler> InitHandlers();

        void    OnConnectedCallback() override;
        int     ReadCallback() override;
        void    SendCallback() override;

        bool    HandlePacketEnumCharacters();
        bool    Handle_CreateNewCharResponse();
        bool    Handle_DeleteCharacterResponse();
        bool    Handle_EnterWorldResponse();
        bool    Handle_ExitWorldResponse();

        // Game handlers, implemeted in World/Handlers/x.cpp
        bool    Handle_PlayerMovementCorrection();
    };
}
}

#endif
