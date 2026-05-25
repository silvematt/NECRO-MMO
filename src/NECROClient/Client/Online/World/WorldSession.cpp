#include "WorldSession.h"
#include "WorldManager.h"
#include "AuthManager.h"
#include "ConsoleLogger.h"
#include "FileLogger.h"
#include "NECROEngine.h"
#include "CharacterData.h"

#include <WorldCodes.h>
#include "AES.h"

namespace NECRO
{
namespace Client
{
    std::unordered_map<uint16_t, WorldHandler> WorldSession::InitHandlers()
    {
        std::unordered_map<uint16_t, WorldHandler> handlers;

        handlers[static_cast<uint16_t>(NECRO::World::PacketIDs::ENUM_CHARACTERS)] = { NECRO::World::WorldSocketStatus::SELECTING_CHARACTERS, NECRO::World::C_PACKET_ENUM_CHARACTERS_INITIAL_SIZE, &WorldSession::HandlePacketEnumCharacters };
        handlers[static_cast<uint16_t>(NECRO::World::PacketIDs::CHAR_CREATE_NEW)] = { NECRO::World::WorldSocketStatus::SELECTING_CHARACTERS, sizeof(NECRO::World::CPacketCreateNewCharResponse), &WorldSession::Handle_CreateNewCharResponse};

        return handlers;
    }
    std::unordered_map<uint16_t, WorldHandler> const Handlers = WorldSession::InitHandlers();


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
        inner << static_cast<uint16_t>(NECRO::World::PacketIDs::AUTH_SESSION);
        inner << static_cast<uint32_t>(authMgr.GetData().iv.prefix);

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

        m_status = NECRO::World::WorldSocketStatus::SELECTING_CHARACTERS;
        LOG_DEBUG("WorldGreet packet sent, my prefix is: {}", authMgr.GetData().iv.prefix);
        c.Log("World Greet sent.");
    }

    int WorldSession::ReadCallback()
    {
        LOG_OK("WorldSession ReadCallback");

        AuthManager& authManager = engine.GetAuthManager();
        NetworkMessage& encryptedPacket = m_inBuffer;

        while (encryptedPacket.GetActiveSize())
        {
            // Try to decrypt a whole packet
            int plaintextLen = encryptedPacket.AESDecrypt(authManager.GetData().sessionKey.data(), nullptr, 0);
            if (plaintextLen <= 0) // if plaintext is 0, it means the client sent an empty packet, shouldn't ever happen
            {
                if (plaintextLen == -1) // Short receive
                    break;

                LOG_WARNING("Decrypt failed for session (code {}). Closing.", plaintextLen);
                return -1;
            }

            // Get the inner decrypted packet
            m_currentDecryptedPacket.Write(encryptedPacket.GetDecryptedPacketPtr(), plaintextLen);

            // If the packet has 1 byte only, we cannot even read the cmd
            if (plaintextLen <= sizeof(uint16_t))
                return -1;

            // Packet is here the decrpyted [CMD | ...] and it arrived fully
            // TODO, it's probably better to do the same memcpy for reading packets instead of SPacketWorldGreet* pkt = reinterpret_cast<SPacketWorldGreet*>(m_currentDecryptedPacket.GetReadPointer());
            uint16_t cmd = 0;
            std::memcpy(&cmd, m_currentDecryptedPacket.GetReadPointer(), sizeof(uint16_t));

            auto it = Handlers.find(cmd);
            if (it == Handlers.end())
            {
                LOG_WARNING("Discarding unknown world packet. CMD: {}", cmd);
                m_currentDecryptedPacket.Clear();
                return -1;
            }

            if (m_status != it->second.status)
            {
                LOG_WARNING("World status mismatch (got {} for cmd {}, expected {}). Closing.",
                    static_cast<int>(m_status), cmd,
                    static_cast<int>(it->second.status));
                return -1;
            }

            LOG_DEBUG("Whole Decrypted Packet Size (with header): {}", m_currentDecryptedPacket.GetActiveSize());

            // No size check is needed, we know we already already have the full packet from the AESDecrypt, and we do data validation in the Handlers.

            try
            {
                // Call the Handler's function and ensure it returns true
                if (!(*this.*it->second.handler)())
                {
                    return -1;
                }
            }
            catch (const std::exception& err)
            {
                LOG_CRITICAL("Exception caught during callback handling. Standard Exception: {}", err.what());
                return -1;
            }
            catch (...)
            {
                LOG_CRITICAL("Exception caught during callback handling. Unknown exception.");
                return -1;
            }

            // Soft clear the current decrypted packet, getting it ready for the next decryption
            m_currentDecryptedPacket.SoftClear();
        }

        return 0;
    }

    void WorldSession::SendCallback()
    {

    }

    bool WorldSession::HandlePacketEnumCharacters()
    {
        LOG_CRITICAL("HandlePacketEnumCharacters...");

        WorldManager& worldManager = engine.GetWorldManager();;

        Console& c = engine.GetConsole();
        NECRO::World::CPacketEnumCharacters* pckData = reinterpret_cast<NECRO::World::CPacketEnumCharacters*>(m_currentDecryptedPacket.GetBasePointer());

        // Check for error - TODO currently the server just drops the connection, if we want graceful shutdown we should do something like this:
        if (pckData->error == static_cast<uint8_t>(NECRO::World::WorldResults::FAILED))
        {
            c.Log("Request failed.");

            // Close
            m_status = NECRO::World::WorldSocketStatus::CLOSED;
            return false;
        }

        if (pckData->error == static_cast<uint8_t>(NECRO::World::WorldResults::NO_CHARACTERS_FOR_THIS_ACCOUNT))
        {
            c.Log("This account has no character, please create one.");

            // Jump to Character Creation Screen
            worldManager.GetData().isAuthed = true;
            return true;
        }
        
        // Pre checks
        if (pckData->charactersNumber == 0 || pckData->charactersNumber > NECRO::World::MAX_CHARACTERS_N_PER_ACCOUNT)
            return false;

        // TODO Check if the sizes match before reading anything (if server lied or not)

        // This is a more involved way of reading the packet, but it allows to read more than one variable length array
        uint8_t* cursor = reinterpret_cast<uint8_t*>(&pckData->characters);
        for (int i = 0; i < pckData->charactersNumber; i++)
        {
            // Read character
            NECRO::CharacterData curCharacter{};

            // id
            std::memcpy(&curCharacter.id, cursor, sizeof(curCharacter.id));
            cursor += sizeof(curCharacter.id);

            // Character Name Length
            std::memcpy(&curCharacter.characterNameLength, cursor, sizeof(curCharacter.characterNameLength));
            cursor += sizeof(curCharacter.characterNameLength);

            // Check on the length
            if (curCharacter.characterNameLength > NECRO::World::CHARACTER_MAX_NAME_LENGTH)
                return false; // malformed packet

            // CharacterName
            curCharacter.characterName.resize(curCharacter.characterNameLength);
            std::memcpy(curCharacter.characterName.data(), cursor, curCharacter.characterNameLength);
            cursor += curCharacter.characterNameLength;

            // Race, class, gender, level
            curCharacter.race = *cursor++;
            curCharacter.gameClass = *cursor++;
            curCharacter.gender = *cursor++;
            curCharacter.level = *cursor++;

            // Xp
            std::memcpy(&curCharacter.xp, cursor, sizeof(curCharacter.xp));
            cursor += sizeof(curCharacter.xp);

            // Zone
            curCharacter.zone = *cursor++;

            // pos x,y,z
            std::memcpy(&curCharacter.pos_x, cursor, sizeof(curCharacter.pos_x));
            cursor += sizeof(curCharacter.pos_x);
            std::memcpy(&curCharacter.pos_y, cursor, sizeof(curCharacter.pos_y));
            cursor += sizeof(curCharacter.pos_y);
            std::memcpy(&curCharacter.pos_z, cursor, sizeof(curCharacter.pos_z));
            cursor += sizeof(curCharacter.pos_z);

            // Cursor is now at the end of the packet or at the start of a new character

            worldManager.GetData().characters.push_back(curCharacter);
        }

        c.Log("===== Characters =====");
        for (size_t i = 0; i < worldManager.GetData().characters.size(); ++i)
        {
            const auto& ch = worldManager.GetData().characters[i];

            c.Log(fmt::format("[{}] ID={}, Name={}, Race={}, Class={}, Gender={}, Level={}, XP={}, Zone={}, Pos=({:.1f},{:.1f},{:.1f})",
                i,
                ch.id,
                ch.characterName,
                ch.race,
                ch.gameClass,
                ch.gender,
                ch.level,
                ch.xp,
                ch.zone,
                ch.pos_x, ch.pos_y, ch.pos_z));
        }

        c.Log("==================");
        c.Log("Characters retrieved.");

        // Jump to Character Selection Screen
        worldManager.GetData().isAuthed = true;

        return true;
    }

    bool WorldSession::Handle_CreateNewCharResponse()
    {
        LOG_CRITICAL("HandlePacketEnumCharacters...");

        Console& c = engine.GetConsole();
        NECRO::World::CPacketCreateNewCharResponse* pckData = reinterpret_cast<NECRO::World::CPacketCreateNewCharResponse*>(m_currentDecryptedPacket.GetBasePointer());

        // Check for error - TODO currently the server just drops the connection, if we want graceful shutdown we should do something like this:
        if (pckData->error == static_cast<uint8_t>(NECRO::World::WorldResults::FAILED))
        {
            c.Log("Request failed.");

            // Close
            m_status = NECRO::World::WorldSocketStatus::CLOSED;
            return false;
        }
        else if (pckData->error == static_cast<uint8_t>(NECRO::World::WorldResults::CHARACTER_NEW_ACCOUNT_HAS_MAX_CHARACTERS_ALLOWED))
        {
            c.Log("This account has already the max num of allowed characters.");
        }
        else if (pckData->error == static_cast<uint8_t>(NECRO::World::WorldResults::CHARACTER_NEW_NAME_ALREADY_IN_USE))
        {
            c.Log("Name is already in use.");
        }
        else if (pckData->error == static_cast<uint8_t>(NECRO::World::WorldResults::CHARACTER_NEW_SUCCESS))
        {
            c.Log("Character Created!");
            // Jump to selection screen
        }

        return true;
    }

}
}
