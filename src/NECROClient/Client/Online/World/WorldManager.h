#ifndef NECRO_WORLD_MANAGER
#define NECRO_WORLD_MANAGER

#include "WorldSession.h"
#include "SocketUtility.h"
#include "CharacterData.h"

#include <memory>
#include <string>
#include <cstdint>

namespace NECRO
{
namespace Client
{
    struct OnlineData
    {
        bool hasConnectedToWorld = false;
        bool isAuthed = false;
        std::vector<NECRO::CharacterData> characters{};
        bool isInWorld = false;
        bool isLeavingWorld = false;
        uint64_t myGuid;

        // Epoch/Ack movement design
        uint32_t m_currentMovSeq = 1; // 0 means none, a correction packet can be issued by the server without the client asking - in that case, the correction packet will have rejectedSeq = 0, so we can be explicit that this is not a correction that derivd from a player movement
        uint32_t m_curretnAckedCorrectionID = 0;

        void Zero()
        {
            hasConnectedToWorld = false;
            isAuthed = false;
            isInWorld = false;
            isLeavingWorld = false;
            myGuid = 0;
            m_currentMovSeq = 1;
            m_curretnAckedCorrectionID = 0;
            characters.clear();
        }
    };

    class WorldManager
    {
    private:
        std::unique_ptr<WorldSession> m_worldSocket;
        bool m_worldSocketConnected = false;
        bool m_isConnecting = false;

        OnlineData m_data;

        // Behavior Related
        float m_timerUpdatePosPacketSend = 99.0f;

    public:
        int     Init();

        void    CreateWorldSocket();

        int     ConnectToWorldServer(const std::string& ip, uint16_t port);

        int     CheckIfWorldConnected(pollfd& pfd);
        int     CheckForIncomingData(pollfd& pfd);
        int     NetworkUpdate();

        void    OnDisconnect();

        int     OnConnectedToWorldServer();

        OnlineData& GetData()
        {
            return m_data;
        }

        void QueuePacket(NetworkMessage&& m)
        {
            m_worldSocket->QueuePacket(std::move(m));
        }

        // Runs after the world has been updated, this is where our own state is replicated to the server
        void    GameUpdate(double deltaTime);

        // Game related, implemented in World/x/x.cpp
        void    SendPlayerMovementUpdate();
    };

}
}

#endif
