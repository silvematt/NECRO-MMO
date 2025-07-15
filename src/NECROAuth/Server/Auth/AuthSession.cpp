#include "AuthSession.h"
#include "NECROServer.h"
#include "AuthCodes.h"
#include "TCPSocketManager.h"

#include <random>
#include <sstream>
#include <iomanip>

#define FMT_HEADER_ONLY
#include <fmt/format.h>

namespace NECRO
{
namespace Auth
{
    std::unordered_map<uint8_t, AuthHandler> AuthSession::InitHandlers()
    {
        std::unordered_map<uint8_t, AuthHandler> handlers;

        handlers[static_cast<int>(PacketIDs::LOGIN_GATHER_INFO)] = { SocketStatus::GATHER_INFO, S_PACKET_AUTH_LOGIN_GATHER_INFO_INITIAL_SIZE, &HandleAuthLoginGatherInfoPacket };
        handlers[static_cast<int>(PacketIDs::LOGIN_ATTEMPT)] = { SocketStatus::LOGIN_ATTEMPT, S_PACKET_AUTH_LOGIN_PROOF_INITIAL_SIZE, &HandleAuthLoginProofPacket };

        return handlers;
    }
    std::unordered_map<uint8_t, AuthHandler> const Handlers = AuthSession::InitHandlers();


    void AuthSession::ReadCallback()
    {
        LOG_DEBUG("AuthSession ReadCallback");

        NetworkMessage& packet = m_inBuffer;

        while (packet.GetActiveSize())
        {
            uint8_t cmd = packet.GetReadPointer()[0]; // read first byte

            auto it = Handlers.find(cmd);
            if (it == Handlers.end())
            {
                // Discard packet, nothing we should handle
                packet.Clear();
                break;
            }

            // Check if the current cmd matches our state
            if (m_status != it->second.status)
            {
                LOG_WARNING(fmt::format("Status mismatch for user: {}. Status is '{}' but should have been '{}'. Closing the connection...", m_data.username, static_cast<int>(m_status), static_cast<int>(it->second.status)));

                Shutdown();
                Close();
                return;
            }

            // Check if the passed packet sizes matches the handler's one, otherwise we're not ready to process this yet
            uint16_t size = uint16_t(it->second.packetSize);
            if (packet.GetActiveSize() < size)
                break;

            // If it's a variable-sized packet, we need to ensure size
            if (cmd == static_cast<int>(PacketIDs::LOGIN_GATHER_INFO))
            {
                SPacketAuthLoginGatherInfo* pcktData = reinterpret_cast<SPacketAuthLoginGatherInfo*>(packet.GetReadPointer());
                size += pcktData->size; // we've read the handler's defined packetSize, so this is safe. Attempt to read the remainder of the packet

                // Check for size
                if (size > S_MAX_ACCEPTED_GATHER_INFO_SIZE)
                {
                    Shutdown();
                    Close();
                    return;
                }
            }
            else if (cmd == static_cast<int>(PacketIDs::LOGIN_ATTEMPT))
            {
                SPacketAuthLoginProof* pcktData = reinterpret_cast<SPacketAuthLoginProof*>(packet.GetReadPointer());
                size += pcktData->size; // we've read the handler's defined packetSize, so this is safe. Attempt to read the remainder of the packet

                // Check for size
                if (size > S_MAX_ACCEPTED_GATHER_INFO_SIZE)
                {
                    Shutdown();
                    Close();
                    return;
                }
            }

            // At this point, ensure the read size matches the whole packet size
            if (packet.GetActiveSize() < size)
                break;  // probably a short receive

            // Call the Handler's function and ensure it returns true
            if (!(*this.*it->second.handler)())
            {
                Close();
                return;
            }

            packet.ReadCompleted(size); // Flag the read as completed, the while will look for remaining packets
        }
    }

    bool AuthSession::HandleAuthLoginGatherInfoPacket()
    {
        SPacketAuthLoginGatherInfo* pcktData = reinterpret_cast<SPacketAuthLoginGatherInfo*>(m_inBuffer.GetReadPointer());

        std::string login((char const*)pcktData->username, pcktData->usernameSize);

        LOG_DEBUG(fmt::format("Handling AuthLoginInfo for user: {}", login));

        // Here we would perform checks such as account exists, banned, suspended, IP locked, region locked, etc.

        // Reply to the client
        Packet packet;

        packet << uint8_t(PacketIDs::LOGIN_GATHER_INFO);

        // Check the DB, see if user exists
        LoginDatabase& db = g_server.GetDirectDB();
        mysqlx::SqlStatement s = db.Prepare(static_cast<int>(LoginDatabaseStatements::SEL_ACCOUNT_ID_BY_NAME));
        s.bind(login);
        mysqlx::SqlResult result = db.Execute(s);
        mysqlx::Row row = result.fetchOne();


        if (!row)
        {
            LOG_INFO("User tried to login with an username that doesn't exist.");
            packet << uint8_t(AuthResults::FAILED_UNKNOWN_ACCOUNT);
        }
        else
        {
            // Check client version with server's client version
            if (pcktData->versionMajor == CLIENT_VERSION_MAJOR && pcktData->versionMinor == CLIENT_VERSION_MINOR && pcktData->versionRevision == CLIENT_VERSION_REVISION)
            {
                packet << uint8_t(AuthResults::SUCCESS);

                m_data.username = login;
                m_data.accountID = row[0];
                LOG_INFO(fmt::format("Account {} has DB AccountID: {}.", login, m_data.accountID));
                m_status = SocketStatus::LOGIN_ATTEMPT;

                // Done, will wait for client's proof packet
            }
            else
            {
                LOG_INFO("User tried to login with an invalid client version.");
                packet << uint8_t(AuthResults::FAILED_WRONG_CLIENT_VERSION);
            }
        }

        NetworkMessage m(packet);

        /* Encryption example
        int res = m.AESEncrypt(data.sessionKey.data(), data.iv, nullptr, 0);
        if (res < 0)
            return false;
        */

        QueuePacket(std::move(m));

        //Send(); packets are sent by checking POLLOUT events in the authSockets, and we check for POLLOUT events only if there are packets written in the outQueue

        return true;
    }

    bool AuthSession::HandleAuthLoginProofPacket()
    {
        SPacketAuthLoginProof* pcktData = reinterpret_cast<SPacketAuthLoginProof*>(m_inBuffer.GetReadPointer());

        LOG_OK(fmt::format("Handling AuthLoginProof for user {}", m_data.username));

        // Reply to the client
        Packet packet;

        packet << uint8_t(PacketIDs::LOGIN_ATTEMPT);

        // Check the DB, see if password is correct
        LoginDatabase& db = g_server.GetDirectDB();
        mysqlx::SqlStatement dStmt1 = db.Prepare(static_cast<int>(LoginDatabaseStatements::CHECK_PASSWORD));
        dStmt1.bind(m_data.accountID);
        mysqlx::SqlResult res = db.Execute(dStmt1);

        mysqlx::Row row = res.fetchOne();

        std::string givenPass((char const*)pcktData->password, pcktData->passwordSize);

        bool authenticated = row[0].get<std::string>() == givenPass;

        auto& dbWorker = g_server.GetDBWorker();
        if (!authenticated)
        {
            LOG_INFO(fmt::format("User {}  tried to send proof with a wrong password.", this->GetRemoteAddressAndPort()));

            // Do an async insert on the DB worker to log that his IP tried to login with a wrong password
            {
                mysqlx::SqlStatement s = dbWorker.Prepare(static_cast<int>(LoginDatabaseStatements::INS_LOG_WRONG_PASSWORD));
                s.bind(this->GetRemoteAddressAndPort());
                s.bind(m_data.username);
                s.bind("WRONG_PASSWORD");
                dbWorker.Enqueue(std::move(s));
            }

            packet << uint8_t(LoginProofResults::FAILED);
            packet << uint16_t(sizeof(CPacketAuthLoginProof) - C_PACKET_AUTH_LOGIN_PROOF_INITIAL_SIZE - AES_128_KEY_SIZE - AES_128_KEY_SIZE); // Adjust the size appropriately
        }
        else
        {
            // Continue login
            packet << uint8_t(LoginProofResults::SUCCESS);

            packet << uint16_t(sizeof(CPacketAuthLoginProof) - C_PACKET_AUTH_LOGIN_PROOF_INITIAL_SIZE); // Adjust the size appropriately, here we send the key

            // Calculate this side's IV, making sure it's different from the client's
            while (pcktData->clientsIVRandomPrefix == m_data.iv.prefix)
                m_data.iv.RandomizePrefix();

            m_data.iv.ResetCounter();

            LOG_INFO(fmt::format("Client's IV Random Prefix: {} | Server's IV Random Prefix: {}", pcktData->clientsIVRandomPrefix, m_data.iv.prefix));

            // Calculate a random session key
            m_data.sessionKey = AES::GenerateSessionKey();

            // Convert sessionKey to hex string in order to print it
            std::ostringstream sessionStrStream;
            for (int i = 0; i < AES_128_KEY_SIZE; ++i)
            {
                sessionStrStream << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(m_data.sessionKey[i]);
            }
            std::string sessionStr = sessionStrStream.str();

            LOG_DEBUG(fmt::format("Session key for user {} is {}.", m_data.username, sessionStr));

            // Write session key to packet
            for (int i = 0; i < AES_128_KEY_SIZE; ++i)
            {
                packet << m_data.sessionKey[i];
            }

            // Create a new greetcode (RAND_bytes). Greetcode is appended with the first packet the client sends to the server, so the server can understand who's he talking to. ONE USE! Server will clear it after first usage
            std::array<uint8_t, AES_128_KEY_SIZE> greetcode = AES::GenerateSessionKey();

            // Delete every previous sessions (if any) of this user, the game server will notice the new connection and kick him the previous client from the game
            // Note: this works because there is only ONE database worker, so we can queue FIFO (if there were multiple workers, the second query (inserting new connection) could have been executed before deleting all the previous sessions, resulting in deleting the new insert as well)
            {
                mysqlx::SqlStatement s = dbWorker.Prepare(static_cast<int>(LoginDatabaseStatements::DEL_PREV_SESSIONS));
                s.bind(m_data.accountID);
                dbWorker.Enqueue(std::move(s));
            }

            // Do an async insert on the DB worker to create a new active_session
            {
                mysqlx::SqlStatement s = dbWorker.Prepare(static_cast<int>(LoginDatabaseStatements::INS_NEW_SESSION));
                s.bind(m_data.accountID);
                s.bind(mysqlx::bytes(m_data.sessionKey.data(), m_data.sessionKey.size()));
                s.bind(this->GetRemoteAddress());
                s.bind(mysqlx::bytes(greetcode.data(), greetcode.size()));
                dbWorker.Enqueue(std::move(s));
            }

            // Write the greetcode to the packet
            for (int i = 0; i < AES_128_KEY_SIZE; ++i)
            {
                packet << greetcode[i];
            }
        }

        NetworkMessage m(packet);
        QueuePacket(std::move(m));

        //Send(); packets are sent by checking POLLOUT events in the authSockets, and we check for POLLOUT events only if there are packets written in the outQueue

        return true;
    }

}
}
