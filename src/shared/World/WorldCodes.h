#pragma once

namespace NECRO
{
namespace World
{
enum class WorldSocketStatus
{
    GATHER_SESSIONKEY,
    GATHER_SESSIONKEY_PENDING,
    SESSIONKEY_GATHERED,
    WAITING_FOR_CHALLENGE,
    AUTHED,
    CLOSED
};

enum class PacketIDs
{
    AUTH_SESSION = 0x00
};

// Packets
#pragma pack(push, 1)

struct SPacketWorldGreet
{
    uint8_t		id;
    uint32_t	clientsPrefix;
};
static_assert(sizeof(SPacketWorldGreet) == (1 + 4), "SPacketWorldGreet size assert failed!");

#pragma pack(pop)
}
}
