#pragma once

#include <unordered_map>
#include <array>
#include <chrono>

#include <mysqlx/xdevapi.h>

#include "TCPSocketBoost.h"
#include "WorldCodes.h"
#include "AES.h"
#include "inerithable_shared_from_this.h"


namespace NECRO
{
namespace World
{
    // GreetCode is valid for 30 seconds from its creation
    inline constexpr const uint32_t GREETCODE_VALIDITY_TIME_WINDOW_SECONDS = 30;

    class WorldSession;

#pragma pack(push, 1)
    struct WorldHandler
    {
        NECRO::World::WorldSocketStatus status;
        size_t packetSize;
        bool (WorldSession::* handler)();
    };
#pragma pack(pop)

    struct WorldSessionData
    {
        std::string username = "";
        uint32_t accountID = 0; // accountid in the database

        std::array<uint8_t, AES_128_KEY_SIZE> greetCode{};
        std::array<uint8_t, AES_128_KEY_SIZE> sessionKey{};
        AES::IV iv;

        uint32_t clientsIVPrefix = 0;
    };

class WorldSession : public TCPSocketBoost, public inheritable_enable_shared_from_this<WorldSession>
{
	using inheritable_enable_shared_from_this<WorldSession>::shared_from_this;

// Members
private:
    // When an encrypted packet arrives [size, iv, tag, ciphertext(innerpacket)], it gets decrypted and the innerpacket gets put into here
    // To read the innerpacket, the handlers use m_currentDecryptedPacket. The content of m_currentDecryptedPacket is valid only for the current handler's execution
    NetworkMessage      m_currentDecryptedPacket;

    WorldSocketStatus   m_status;
    WorldSessionData    m_data;

    bool                m_closeAfterSend; // when this is true, the SendCallback will close the socket. Used to close connection as soon as possible when a client is not valid

    std::chrono::steady_clock::time_point   m_lastActivity;
    uint32_t m_packetsProcessed = 0;
    
public:
    WorldSession(tcp::socket&& insocket) : TCPSocketBoost(std::move(insocket)), m_status(WorldSocketStatus::GATHER_SESSIONKEY), m_closeAfterSend(false)
    {
    }

    static std::unordered_map<uint16_t, WorldHandler> InitHandlers();

    // This runs in the NetworkThread that possess this socket
    int     Update(std::chrono::steady_clock::time_point now) override;

    int     AsyncReadCallback() override;
    void    AsyncWriteCallback() override;

    bool    DBCallback_GreetcodeLookup(uint32_t ec, std::vector<mysqlx::SqlResult>& result);
    bool    HandleGreetPacket();
    bool    DBCallback_HandleGreetPacket(uint32_t ec, std::vector<mysqlx::SqlResult>& result);

};
}
}
