#pragma once

#include "TCPSocket.h"
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
class WorldSession : public TCPSocketBoost, public inheritable_enable_shared_from_this<WorldSession>
{
	using inheritable_enable_shared_from_this<WorldSession>::shared_from_this;

// Members
private:
    WorldSocketStatus   m_status;
    bool                m_closeAfterSend; // when this is true, the SendCallback will close the socket. Used to close connection as soon as possible when a client is not valid

    uint32_t            m_packetsProcessed = 0;

public:
    WorldSession(tcp::socket&& insocket) : TCPSocketBoost(std::move(insocket)), m_status(WorldSocketStatus::GATHER_INFO), m_closeAfterSend(false)
    {
    }

    // This runs in the NetworkThread that possess this socket
    int     Update(std::chrono::steady_clock::time_point now) override;

    int     AsyncReadCallback() override;
    void    AsyncWriteCallback() override;
};
}
}
