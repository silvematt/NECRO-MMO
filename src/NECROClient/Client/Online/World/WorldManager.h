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
        std::vector<NECRO::CharacterData> characters{};

        void Zero()
        {
            hasConnectedToWorld = false;
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

    public:
        int Init();

        void CreateWorldSocket();

        int ConnectToWorldServer(const std::string& ip, uint16_t port);

        int CheckIfWorldConnected(pollfd& pfd);
        int CheckForIncomingData(pollfd& pfd);
        int NetworkUpdate();

        void OnDisconnect();

        int OnConnectedToWorldServer();

        OnlineData& GetData()
        {
            return m_data;
        }
    };

}
}

#endif
