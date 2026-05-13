#pragma once

namespace NECRO
{
namespace World
{
enum class WorldSocketStatus
{
    GATHER_SESSIONKEY = 0,
    GATHER_SESSIONKEY_PENDING,
    SESSIONKEY_GATHERED,
    SELECTING_CHARACTERS,
    AUTHED,
    CLOSED
};

enum class PacketIDs : uint16_t
{
    AUTH_SESSION = 0x00,
    ENUM_CHARACTERS = 0x01
};

//--------------------------------------------------------------------------------------------
// Results to send as payload to tell the client what happened as a result of the command 
//--------------------------------------------------------------------------------------------
enum class WorldResults : uint16_t
{
    FAILED = 0x00,
    SUCCESS = 0x01,
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

// Character data
struct CharacterData
{
    uint16_t id;
    
    uint8_t characterNameLength;
    uint8_t characterName[1];

    uint8_t race;
    uint8_t gameClass;
    uint8_t gender;

    uint8_t level;
    uint32_t xp;

    uint8_t zone;
    float_t pos_x;
    float_t pos_y;
    float_t pos_z;
};

struct CPacketEnumCharacters
{
    uint16_t    id;
    uint8_t     errorCode; // WorldResults
    uint16_t    size;

    uint8_t         charactersNumber;
    CharacterData   characters[];
};
static_assert(sizeof(CPacketEnumCharacters) == (2 + 1 + 2 + 1), "CPacketEnumCharacters size assert failed!");
inline constexpr int C_PACKET_GATHER_REALMLIST_INITIAL_SIZE = 5; // this represent the fixed portion of this packet, which needs to be read to at least identify the packet
inline constexpr int MAX_CHARACTERS_N = 10;

#pragma pack(pop)
}
}
