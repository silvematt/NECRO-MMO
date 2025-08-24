#include "AuthSession.h"
#include "NECROServer.h"
#include "AuthCodes.h"
#include "TCPSocketManager.h"
#include "DBRequest.h"

#include <random>
#include <sstream>
#include <iomanip>

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


    int AuthSession::ReadCallback()
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
                LOG_DEBUG("Discarding the packet.");
                packet.Clear();
                return -1;
                //break; TODO, adding some kind of tolerance may be good for messages that gets corrupted in flight
            }

            // Check if the current cmd matches our state
            if (m_status != it->second.status)
            {
                LOG_WARNING("Status mismatch for user: {}. Status is '{}' but should have been '{}'. Closing the connection...", m_data.username, static_cast<int>(m_status), static_cast<int>(it->second.status));

                return -1;
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
                    return -1;
                }
            }
            else if (cmd == static_cast<int>(PacketIDs::LOGIN_ATTEMPT))
            {
                SPacketAuthLoginProof* pcktData = reinterpret_cast<SPacketAuthLoginProof*>(packet.GetReadPointer());
                size += pcktData->size; // we've read the handler's defined packetSize, so this is safe. Attempt to read the remainder of the packet

                // Check for size
                if (size > S_MAX_ACCEPTED_GATHER_INFO_SIZE)
                {
                    return - 1;
                }
            }

            // At this point, ensure the read size matches the whole packet size
            if (packet.GetActiveSize() < size)
                break;  // probably a short receive

            // Call the Handler's function and ensure it returns true
            if (!(*this.*it->second.handler)())
            {
                return -1;
            }

            packet.ReadCompleted(size); // Flag the read as completed, the while will look for remaining packets
        }

        return 0;
    }

    bool AuthSession::HandleAuthLoginGatherInfoPacket()
    {
        SPacketAuthLoginGatherInfo* pcktData = reinterpret_cast<SPacketAuthLoginGatherInfo*>(m_inBuffer.GetReadPointer());

        // Pre-checks
        // Check for username size
        if (pcktData->usernameSize > Auth::MAX_USERNAME_LENGTH)
            return false;

        // Check for username value (input validation)
        for(int i = 0; i < pcktData->usernameSize; i++)
            if(!std::isalnum(pcktData->username[i]))
                return false;

        // Fill data
        std::string login((char const*)pcktData->username, pcktData->usernameSize);
        m_data.username = login;

        m_data.versionMajor = pcktData->versionMajor;
        m_data.versionMinor = pcktData->versionMinor;
        m_data.versionRevision = pcktData->versionRevision;

        LOG_DEBUG("Handling AuthLoginInfo for user: {}", m_data.username);

        // Here we would perform checks such as account exists, banned, suspended, IP locked, region locked, etc.
        auto& dbworker = Server::Instance().GetDBWorker();
        {
            DBRequest req(static_cast<int>(LoginDatabaseStatements::SEL_ACCOUNT_ID_BY_NAME), false);
            req.m_bindParams.push_back(m_data.username);

            // The callback needs to ensure the object still exists, as it may be deleted by the main thread while the request is being processed
            std::weak_ptr<AuthSession> weakSelf = shared_from_this();
            req.m_callback = [weakSelf](mysqlx::SqlResult& res)
            {
                if (auto lockedSelf = weakSelf.lock()) 
                    return lockedSelf->DBCallback_AuthLoginGatherInfoPacket(res);

                return false; // AuthSession is destroyed (disconnect)
            };

            req.m_cancelToken = weakSelf;
            req.m_noticeFunc = [weakSelf]() 
            {
                if (auto self = weakSelf.lock()) 
                {
                    Server::Instance().GetSocketManager().WakeUp();
                }
            };
            dbworker.Enqueue(std::move(req));
        }

        return true;
    }

    bool AuthSession::DBCallback_AuthLoginGatherInfoPacket(mysqlx::SqlResult& result)
    {
        LOG_CRITICAL("Handling DBCallback_AuthLoginGatherInfoPacket for user {}!!", m_data.username);

        // Reply to the client
        Packet packet;
        packet << uint8_t(PacketIDs::LOGIN_GATHER_INFO);

        mysqlx::Row row = result.fetchOne();

        if (!row)
        {
            LOG_INFO("User tried to login with an username that doesn't exist.");

            m_closeAfterSend = true;

            packet << uint8_t(AuthResults::FAILED_UNKNOWN_ACCOUNT);
        }
        else
        {
            auto& serverSettings = Server::Instance().GetSettings();
            
            // Check client version with server's client version
            if (m_data.versionMajor == serverSettings.CLIENT_VERSION_MAJOR && m_data.versionMinor == serverSettings.CLIENT_VERSION_MINOR && m_data.versionRevision == serverSettings.CLIENT_VERSION_REVISION)
            {
                packet << uint8_t(AuthResults::SUCCESS);

                m_data.accountID = row[0];
                LOG_INFO("Account {} has DB AccountID: {}.", m_data.username, m_data.accountID);
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

        // Pre-checks
        // Check for password size
        if (pcktData->passwordSize > Auth::MAX_PASSWORD_LENGTH)
            return false;

        // Check for password value (input validation)
        for (int i = 0; i < pcktData->passwordSize; i++)
            if (!std::isalnum(pcktData->password[i]))
                return false;

        LOG_OK("Handling AuthLoginProof for user {}", m_data.username);

        std::string p((char const*)pcktData->password, pcktData->passwordSize);
        m_data.pass = p;
        m_data.randIVPrefix = pcktData->clientsIVRandomPrefix;

        auto& dbworker = Server::Instance().GetDBWorker();
        {
            DBRequest req(static_cast<int>(LoginDatabaseStatements::CHECK_PASSWORD), false);
            req.m_bindParams.push_back(m_data.accountID);

            // The callback needs to ensure the object still exists, as it may be deleted by the main thread while the request is being processed
            std::weak_ptr<AuthSession> weakSelf = shared_from_this();
            req.m_callback = [weakSelf](mysqlx::SqlResult& res) 
            {
                if (auto lockedSelf = weakSelf.lock()) 
                    return lockedSelf->DBCallback_AuthLoginProofPacket(res);

                return false; // AuthSession is destroyed (disconnect)
            };

            req.m_cancelToken = weakSelf;
            req.m_noticeFunc = [weakSelf]()
                {
                    if (auto self = weakSelf.lock())
                    {
                        Server::Instance().GetSocketManager().WakeUp();
                    }
                };
            dbworker.Enqueue(std::move(req));
        }

        return true;
    }

    bool AuthSession::DBCallback_AuthLoginProofPacket(mysqlx::SqlResult& result)
    {
        LOG_CRITICAL("Handling DBCallback_AuthLoginProofPacket for user {}!!", m_data.username);

        // Reply to the client
        Packet packet;

        packet << uint8_t(PacketIDs::LOGIN_ATTEMPT);

        mysqlx::Row row = result.fetchOne();

        LOG_CRITICAL("my {} db {}", m_data.pass, row[0].get<std::string>());

        bool authenticated = row[0].get<std::string>() == m_data.pass;

        auto& dbworker = Server::Instance().GetDBWorker();
        if (!authenticated)
        {
            LOG_INFO("User {}  tried to send proof with a wrong password.", this->GetRemoteAddressAndPort());

            // Do an async insert on the DB worker to log that his IP tried to login with a wrong password
            {
                DBRequest req(static_cast<int>(LoginDatabaseStatements::INS_LOG_WRONG_PASSWORD), true);
                req.m_bindParams.push_back(this->GetRemoteAddressAndPort());
                req.m_bindParams.push_back(m_data.username);
                req.m_bindParams.push_back("WRONG_PASSWORD");
                dbworker.Enqueue(std::move(req));
            }

            packet << uint8_t(LoginProofResults::FAILED);
            m_closeAfterSend = true;
            packet << uint16_t(sizeof(CPacketAuthLoginProof) - C_PACKET_AUTH_LOGIN_PROOF_INITIAL_SIZE - AES_128_KEY_SIZE - AES_128_KEY_SIZE); // Adjust the size appropriately
        }
        else
        {
            // Continue login
            packet << uint8_t(LoginProofResults::SUCCESS);

            packet << uint16_t(sizeof(CPacketAuthLoginProof) - C_PACKET_AUTH_LOGIN_PROOF_INITIAL_SIZE); // Adjust the size appropriately, here we send the key

            // Calculate this side's IV, making sure it's different from the client's
            while (m_data.randIVPrefix == m_data.iv.prefix)
                m_data.iv.RandomizePrefix();

            m_data.iv.ResetCounter();

            LOG_INFO("Client's IV Random Prefix: {} | Server's IV Random Prefix: {}", m_data.randIVPrefix, m_data.iv.prefix);

            // Calculate a random session key
            m_data.sessionKey = AES::GenerateSessionKey();

            // Convert sessionKey to hex string in order to print it
            std::ostringstream sessionStrStream;
            for (int i = 0; i < AES_128_KEY_SIZE; ++i)
            {
                sessionStrStream << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(m_data.sessionKey[i]);
            }
            std::string sessionStr = sessionStrStream.str();

            LOG_DEBUG("Session key for user {} is {}.", m_data.username, sessionStr);

            // Write session key to packet
            for (int i = 0; i < AES_128_KEY_SIZE; ++i)
            {
                packet << m_data.sessionKey[i];
            }

            // Create a new greetcode (RAND_bytes). Greetcode is appended with the first packet the client sends to the server, so the server can understand who's he talking to. ONE USE! Server will clear it after first usage
            std::array<uint8_t, AES_128_KEY_SIZE> greetcode = AES::GenerateSessionKey();

            // Delete every previous sessions (if any) of this user, the game server will notice the new connection and kick him the previous client from the game
            // Note: this works because there is only ONE database worker, so we can queue FIFO (if there were multiple workers, the second query (inserting new connection) could have been executed before deleting all the previous sessions, resulting in deleting the new insert as well)
            // TODO transaction or new callback
            {
                DBRequest req(static_cast<int>(LoginDatabaseStatements::DEL_PREV_SESSIONS), true);
                req.m_bindParams.push_back(m_data.accountID);
                dbworker.Enqueue(std::move(req));
            }

            // Do an async insert on the DB worker to create a new active_session
            {
                DBRequest req(static_cast<int>(LoginDatabaseStatements::INS_NEW_SESSION), true);
                req.m_bindParams.push_back(m_data.accountID);
                req.m_bindParams.push_back(mysqlx::bytes(m_data.sessionKey.data(), m_data.sessionKey.size()));
                req.m_bindParams.push_back(this->GetRemoteAddress());
                req.m_bindParams.push_back(mysqlx::bytes(greetcode.data(), greetcode.size()));
                dbworker.Enqueue(std::move(req));
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

    void AuthSession::SendCallback()
    {
        if (m_closeAfterSend && m_outQueue.size() == 0)
        {
            LOG_DEBUG("Send Callback called on m_closeAfterSend.");
            closesocket(GetSocketFD()); // TODO, this doesn't work correctly. Client should receive the packet and then the termination signal, but it doesn't happen as of now
        }
    }

}
}
