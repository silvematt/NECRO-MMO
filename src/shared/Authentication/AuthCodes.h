#pragma once

#include "AES.h"

namespace NECRO
{
namespace Auth
{
    // Status of the AuthSessions during communication
    enum class SocketStatus
    {
        HANDSHAKING = 0,
        GATHER_INFO,
        GATHER_INFO_PENDING,
        LOGIN_ATTEMPT,
        LOGIN_ATTEMPT_PENDING,
        AUTHED,
        GATHER_REALMLIST_PENDING,
        CLOSED
    };

    //----------------------------------------------------------------------------------------------------
    // Define packets structures
    //----------------------------------------------------------------------------------------------------
    enum class PacketIDs : uint8_t
    {
        LOGIN_GATHER_INFO = 0x00,
        LOGIN_ATTEMPT = 0x01,
        LOGIN_GATHER_REALMLIST = 0x02
    };

    //--------------------------------------------------------------------------------------------
    // Results to send as payload to tell the client what happened as a result of the command 
    //--------------------------------------------------------------------------------------------
    enum class AuthResults : uint8_t
    {
        SUCCESS = 0x00,
        FAILED_WRONG_CLIENT_VERSION,
        FAILED_UNKNOWN_ACCOUNT,
        FAILED_ACCOUNT_BANNED,
        FAILED_WRONG_PASSWORD,
    };

    enum class LoginProofResults : uint8_t
    {
        SUCCESS = 0x00,
        FAILED = 0X01
    };


    // Packets
    #pragma pack(push, 1)

    // -------------------------------------------------------------------------------------------------------
    // When defining packets: 
    // 1) S prefix means (for)Server, so it's a packet that the server will receive and the client will send
    // 2) C prefix means (for)Client, so it's a packet that the client will receive and the server will send
    // -------------------------------------------------------------------------------------------------------

    struct SPacketAuthLoginGatherInfo
    {
        uint8_t		id;
        uint8_t		error;
        uint16_t	size;

        uint8_t		versionMajor;
        uint8_t		versionMinor;
        uint8_t		versionRevision;

        uint8_t		usernameSize;
        uint8_t		username[1];
    };
    static_assert(sizeof(SPacketAuthLoginGatherInfo) == (1 + 1 + 2 + 1 + 1 + 1 + 1 + 1), "SPacketAuthLoginGatherInfo size assert failed!");
    inline constexpr int MAX_USERNAME_LENGTH = 16;
    inline constexpr int S_MAX_ACCEPTED_GATHER_INFO_SIZE = ((sizeof(SPacketAuthLoginGatherInfo) - 1) + MAX_USERNAME_LENGTH);    // 16 is username length
    inline constexpr int S_PACKET_AUTH_LOGIN_GATHER_INFO_INITIAL_SIZE = 4; // this represent the fixed portion of this packet, which needs to be read to at least identify the packet

    struct CPacketAuthLoginGatherInfo
    {
        uint8_t		id;
        uint8_t		error;
        uint16_t    size;

        // Server Challenge 
        uint8_t     challenge[AES_128_KEY_SIZE];
        uint8_t     difficulty;
    };
    static_assert(sizeof(CPacketAuthLoginGatherInfo) == (1 + 1 + 2 + 16 + 1), "CPacketAuthLoginGatherInfo size assert failed!");
    inline constexpr int C_PACKET_AUTH_LOGIN_GATHER_INFO_INITIAL_SIZE = 4; // this represent the fixed portion of this packet, which needs to be read to at least identify the packet

    struct SPacketAuthLoginProof
    {
        uint8_t		id;
        uint8_t		error;
        uint16_t    size;

        uint64_t    answer;
        uint32_t    clientsIVRandomPrefix;

        uint8_t     passwordSize;
        uint8_t     password[1];
    };
    static_assert(sizeof(SPacketAuthLoginProof) == (1 + 1 + 2 + 8 + 4 + 1 + 1), "SPacketAuthLoginProof size assert failed!");
    inline constexpr int MAX_PASSWORD_LENGTH = 16; 
    inline constexpr int S_MAX_ACCEPTED_AUTH_LOGIN_PROOF_SIZE = ((sizeof(SPacketAuthLoginProof)-1) + MAX_PASSWORD_LENGTH); // 16 is username length
    inline constexpr int S_PACKET_AUTH_LOGIN_PROOF_INITIAL_SIZE = 4; // this represent the fixed portion of this packet, which needs to be read to at least identify the packet

    struct CPacketAuthLoginProof
    {
        uint8_t		id;
        uint8_t		error;
        uint16_t    size;

        uint8_t     sessionKey[AES_128_KEY_SIZE];
        uint8_t     greetcode[AES_128_KEY_SIZE];
    };
    static_assert(sizeof(CPacketAuthLoginProof) == (1 + 1 + 2 + AES_128_KEY_SIZE + AES_128_KEY_SIZE), "CPacketAuthLoginProof size assert failed!");
    inline constexpr int C_PACKET_AUTH_LOGIN_PROOF_INITIAL_SIZE = 4; // this represent the fixed portion of this packet, which needs to be read to at least identify the packet

    struct SPacketGatherRealmlist
    {
        uint8_t id;
        uint8_t error;
    };
    static_assert(sizeof(SPacketGatherRealmlist) == (1 + 1), "SPacketGatherRealmlist size assert failed!");


    struct RealmDataOnWire
    {
        uint32_t id;

        uint8_t status;

        uint8_t ipAddress[4];
        uint16_t port;

        uint8_t nameSize;
        uint8_t name[1];
    };
    inline constexpr int REALM_MAX_NAME_SIZE = 32;

    struct CPacketGatherRealmlist
    {
        uint8_t id;
        uint8_t error;
        uint16_t size;

        uint32_t        numOfRealms;
        RealmDataOnWire bytes[];
    };

    static_assert(sizeof(CPacketGatherRealmlist) == (1 + 1 + 2 + 4), "CPacketGatherRealmlist size assert failed!");
    inline constexpr int C_PACKET_GATHER_REALMLIST_INITIAL_SIZE = 4; // this represent the fixed portion of this packet, which needs to be read to at least identify the packet
    inline constexpr int MAX_REALMS_N = 32;
    #pragma pack(pop)
}
}
