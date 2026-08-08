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
    ENTERING_WORLD,
    IN_WORLD,
    LEAVING_WORLD,
    CLOSED
};

enum class PacketIDs : uint16_t
{
    AUTH_SESSION = 0,
    ENUM_CHARACTERS,
    CHAR_CREATE_NEW,
    CHAR_DELETE_CHARACTER,
    ENTER_WORLD,
    EXIT_WORLD
};

//--------------------------------------------------------------------------------------------
// Results to send as payload to tell the client what happened as a result of the command 
//--------------------------------------------------------------------------------------------
enum class WorldResults : uint16_t
{
    FAILED = 0,
    SUCCESS,
    NO_CHARACTERS_FOR_THIS_ACCOUNT,
    CHARACTER_NEW_ACCOUNT_HAS_MAX_CHARACTERS_ALLOWED,
    CHARACTER_NEW_NAME_ALREADY_IN_USE,
    CHARACTER_NEW_SUCCESS,
    CHARACTER_NOT_FOUND,
    CHARACTER_DOES_NOT_BELONG_TO_ACCOUNT
};

// Packets
#pragma pack(push, 1)

// Authentication:
// Clients sends the greet packet, [GREETCODE | ENCRYPTED_PACKET] server decrypts it
// Server sends CEnumCharactersPacket as [ENCRYPTED_PACKET] ([PCKT_SIZE | IV | TAG | CIPHERTEXT])
// Client processes decrypts and processes the packet, if this fails, the client wasn't legit (may have stolen the greetcode)
struct SPacketWorldGreet
{
    uint16_t	id;
    uint32_t	clientsPrefix;
};
static_assert(sizeof(SPacketWorldGreet) == (2 + 4), "SPacketWorldGreet size assert failed!");

// Character data on the wire
struct CharacterDataOnWire
{
    uint32_t id;
    
    uint8_t characterNameLength;
    uint8_t characterName[1];

    uint8_t race;
    uint8_t gameClass;
    uint8_t gender;

    uint8_t level;
    uint32_t xp;

    uint32_t zone;
    float_t pos_x;
    float_t pos_y;
    float_t pos_z;
};

// Client requests enum of character, for example, when he goes from the character creation screen to the selection screen
struct SPacketEnumCharacter
{
    uint16_t    id;
};
static_assert(sizeof(SPacketEnumCharacter) == (2), "SPacketEnumCharacter size assert failed!");
inline constexpr int S_PACKET_ENUM_CHAR_INITIAL_SIZE = 2; // this represent the fixed portion of this packet, which needs to be read to at least identify the packet

struct CPacketEnumCharacters
{
    uint16_t    id;
    uint8_t     error; // WorldResults

    uint8_t             charactersNumber;
    CharacterDataOnWire characters[];
};
static_assert(sizeof(CPacketEnumCharacters) == (2 + 1 + 1), "CPacketEnumCharacters size assert failed!");
inline constexpr int C_PACKET_ENUM_CHARACTERS_INITIAL_SIZE = 4; // this represent the fixed portion of this packet, which needs to be read to at least identify the packet
inline constexpr int CHARACTER_MAX_NAME_LENGTH = 12;
inline constexpr int MAX_CHARACTERS_N_PER_ACCOUNT = 10;

// Client requests new character creation
struct SPacketCreateNewChar
{
    uint16_t    id;
    uint16_t    size;

    uint8_t     race;
    uint8_t     charClass;
    uint8_t     gender;
    
    uint8_t     characterNameLength;
    uint8_t     characterName[1];
};
static_assert(sizeof(SPacketCreateNewChar) == (2 + 2 + 1 + 1 + 1 + 1 + 1), "SPacketCreateNewChar size assert failed!");
inline constexpr int S_PACKET_CREATE_NEW_CHAR_INITIAL_SIZE = 4; // this represent the fixed portion of this packet, which needs to be read to at least identify the packet

// Server replies to SPacketCreateNewChar
struct CPacketCreateNewCharResponse
{
    uint16_t    id;
    uint8_t     error;
};
static_assert(sizeof(CPacketCreateNewCharResponse) == (2 + 1), "CPacketCreateNewCharResponse size assert failed!");

// Client requests deletion of character
struct SPacketDeleteCharacter
{
    uint16_t    id;
    uint16_t    size;

    uint32_t    characterID;
    uint8_t     characterNameLength;
    uint8_t     characterName[1];
};
static_assert(sizeof(SPacketDeleteCharacter) == (2 + 2 + 4 + 1 + 1), "CPacketCreateNewCharResponse size assert failed!");
inline constexpr int S_PACKET_DELETE_CHAR_INITIAL_SIZE = 4; // this represent the fixed portion of this packet, which needs to be read to at least identify the packet

// Server replies to SPacketDeleteCharacter
struct CPacketDeleteCharacterResponse
{
    uint16_t    id;
    uint8_t     error;
};
static_assert(sizeof(CPacketDeleteCharacterResponse) == (2 + 1), "CPacketDeleteCharacterResponse size assert failed!");

// Client asks the server to enter the world
struct SPacketEnterWorld
{
    uint16_t id;
    uint32_t characterID; // character that the player wants to enter with
};
static_assert(sizeof(SPacketEnterWorld) == (2 + 4), "SPacketEnterWorld size assert failed!");

// Server replies to the client that he may (or may not) enter
struct CPacketEnterWorld
{
    uint16_t id;
    uint8_t error;

    // Server tells client the information to spawn in the world
    uint64_t guid;// the guid the simulation assigned to our character, unique within this run of the server

    uint32_t mapID;
    float_t posX;
    float_t posY;
};
static_assert(sizeof(CPacketEnterWorld) == (2 + 1 + 8 + 4 + 4 + 4), "CPacketEnterWorld size assert failed!");
inline constexpr int C_PACKET_ENTER_WORLD_INITIAL_SIZE = 3;

// Client requests to exit the worldserver
struct SPacketExitWorld
{
    uint16_t id;
};
static_assert(sizeof(SPacketExitWorld) == (2), "SPacketLeaveWorld size assert failed!");

// Client requests to exit the worldserver
struct CPacketExitWorld
{
    uint16_t id;
    uint8_t error;
};
static_assert(sizeof(CPacketExitWorld) == (2 + 1), "CPacketExitWorld size assert failed!");

#pragma pack(pop)
}
}
