#include "WorldSession.h"
#include "WorldManager.h"
#include "AuthManager.h"
#include "ConsoleLogger.h"
#include "FileLogger.h"
#include "NECROEngine.h"

#include <WorldCodes.h>
#include "AES.h"

namespace NECRO
{
namespace Client
{
    std::unordered_map<uint8_t, WorldHandler> WorldSession::InitHandlers()
    {
        std::unordered_map<uint8_t, WorldHandler> handlers;

        return handlers;
    }
    std::unordered_map<uint8_t, WorldHandler> const Handlers = WorldSession::InitHandlers();


    void WorldSession::OnConnectedCallback()
    {
        AuthManager& authMgr = engine.GetAuthManager();
        Console& c = engine.GetConsole();

        // Pick a fresh IV prefix for this world session and reset the counter
        // The WorldServer gets this prefix so it can generate a different one
        authMgr.GetData().iv.RandomizePrefix();
        authMgr.GetData().iv.ResetCounter();

        // Inner (encrypted) packet: [ID | clientsIVRandomPrefix]
        Packet inner;
        inner << uint8_t(NECRO::World::PacketIDs::AUTH_SESSION);
        inner << uint32_t(authMgr.GetData().iv.prefix);

        NetworkMessage encrypted(std::move(inner));
        if (encrypted.AESEncrypt(authMgr.GetData().sessionKey.data(), authMgr.GetData().iv, nullptr, 0) < 0)
        {
            LOG_ERROR("Failed to encrypt world greet packet.");
            c.Log("Failed to encrypt world greet packet.");
            return;
        }

        // Write packet [GREETCODE (plaintext) | ENCRYPTED_PACKET] TODO: define the packet in a struct
        Packet packet;
        packet.Append(authMgr.GetData().greetcode.data(), AES_128_KEY_SIZE);
        packet.Append(encrypted.GetReadPointer(), encrypted.GetActiveSize());

        NetworkMessage m(std::move(packet));
        QueuePacket(std::move(m));

        m_status = NECRO::World::WorldSocketStatus::GATHER_SESSIONKEY_PENDING;
        LOG_DEBUG("WorldGreet packet sent, my prefix is: {}", authMgr.GetData().iv.prefix);
        c.Log("World Greet sent.");
    }

    int WorldSession::ReadCallback()
    {
        LOG_OK("WorldSession ReadCallback");

        NetworkMessage& packet = m_inBuffer;

        while (packet.GetActiveSize())
        {
            uint8_t cmd = packet.GetReadPointer()[0];

            auto it = Handlers.find(cmd);
            if (it == Handlers.end())
            {
                LOG_WARNING("Discarding world packet. CMD: {}", cmd);

                packet.Clear();
                break;
            }

            if (m_status != it->second.status)
            {
                LOG_WARNING("World status mismatch. Status is: '{}' but should have been '{}'. Closing the connection.", static_cast<int>(m_status), static_cast<int>(it->second.status));

                Shutdown();
                Close();
                return -1;
            }

            uint16_t size = uint16_t(it->second.packetSize);
            if (packet.GetActiveSize() < size)
                break;

            if (!(*this.*it->second.handler)())
            {
                Close();
                return -1;
            }

            packet.ReadCompleted(size);
        }

        return 0;
    }

    void WorldSession::SendCallback()
    {

    }

}
}
