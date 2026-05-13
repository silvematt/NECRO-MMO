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

enum class PacketIDs : uint16_t
{
    AUTH_SESSION = 0x00,
    ENUM_CHARACTERS = 0x01
};

// Packets
#pragma pack(push, 1)

// Authentication:
// Clients sends the greet packet, [GREETCODE | ENCRYPTED_PACKET] server decrypts it
// Server sends CEnumCharactersPacket as [ENCRYPTED_PACKET] ([PCKT_SIZE | IV | TAG | CIPHERTEXT])
// Client processes decrypts and processes the packet, if this fails, the client wasn't legit (may have stolen the greetcode(
struct SPacketWorldGreet
{
    uint16_t	id;
    uint32_t	clientsPrefix;
};
static_assert(sizeof(SPacketWorldGreet) == (2 + 4), "SPacketWorldGreet size assert failed!");

#pragma pack(pop)
}
}
