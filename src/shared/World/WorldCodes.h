#pragma once

namespace NECRO
{
namespace World
{
enum class WorldSocketStatus
{
    GATHER_INFO,
    GATHER_INFO_PENDING,
    AUTHED,
    CLOSED
};

enum class PacketIDs
{
    AUTH_SESSION = 0x00
};

}
}