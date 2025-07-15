#ifndef AUTH_CODES_H
#define AUTH_CODES_H

namespace NECRO
{
namespace Auth
{
    // Status of the sockets during communication
    enum class SocketStatus
    {
        GATHER_INFO = 0,
        LOGIN_ATTEMPT,
        AUTHED,
        CLOSED
    };

    //----------------------------------------------------------------------------------------------------
    // Define packets structures
    //----------------------------------------------------------------------------------------------------
    enum class PacketIDs
    {
        LOGIN_GATHER_INFO = 0x00,
        LOGIN_ATTEMPT = 0x01,
        CONFIRM = 0x02
    };

    //--------------------------------------------------------------------------------------------
    // Results to send as payload to tell the client what happened as a result of the command 
    //--------------------------------------------------------------------------------------------
    enum class AuthResults
    {
        SUCCESS = 0x00,
        FAILED_UNKNOWN_ACCOUNT = 0x01,
        FAILED_ACCOUNT_BANNED = 0x02,
        FAILED_WRONG_PASSWORD = 0x03,
        FAILED_WRONG_CLIENT_VERSION = 0x04,
        FAILED_USERNAME_IN_USE = 0x05		// before we implement database and we'll just have username uniqueness per session
    };


    enum class LoginProofResults
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
    inline constexpr int MAX_USERNAME_LENGTH  = 16 - 1;                                                                   // 1 is already in packet username[1] 
    inline constexpr int S_MAX_ACCEPTED_GATHER_INFO_SIZE = (sizeof(SPacketAuthLoginGatherInfo) + MAX_USERNAME_LENGTH);    // 16 is username length
    inline constexpr int S_PACKET_AUTH_LOGIN_GATHER_INFO_INITIAL_SIZE = 4; // this represent the fixed portion of this packet, which needs to be read to at least identify the packet


    struct CPacketAuthLoginGatherInfo
    {
        uint8_t		id;
        uint8_t		error;
    };
    static_assert(sizeof(CPacketAuthLoginGatherInfo) == (1 + 1), "CPacketAuthLoginGatherInfo size assert failed!");

    struct SPacketAuthLoginProof
    {
        uint8_t		id;
        uint8_t		error;
        uint16_t    size;

        uint32_t    clientsIVRandomPrefix;

        uint8_t passwordSize;
        uint8_t password[1];
    };
    static_assert(sizeof(SPacketAuthLoginProof) == (1 + 1 + 2 + 4 + 1 + 1), "SPacketAuthLoginProof size assert failed!");
    inline constexpr int MAX_PASSWORD_LENGTH = 16 - 1; // 1 is already in packet password[1] 
    inline constexpr int S_MAX_ACCEPTED_AUTH_LOGIN_PROOF_SIZE = (sizeof(SPacketAuthLoginProof) + MAX_PASSWORD_LENGTH); // 16 is username length
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

    #pragma pack(pop)

}
}

#endif
