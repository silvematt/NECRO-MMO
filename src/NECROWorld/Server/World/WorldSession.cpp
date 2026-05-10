#include "WorldSession.h"
#include "NECROWorld.h"

namespace NECRO
{
namespace World
{

    std::unordered_map<uint8_t, WorldHandler> WorldSession::InitHandlers()
    {
        std::unordered_map<uint8_t, WorldHandler> handlers;

        handlers[static_cast<int>(World::PacketIDs::AUTH_SESSION)] = { NECRO::World::WorldSocketStatus::SESSIONKEY_GATHERED, sizeof(NECRO::World::SPacketWorldGreet) , &HandleGreetPacket };

        return handlers;
    }
    std::unordered_map<uint8_t, WorldHandler> const Handlers = WorldSession::InitHandlers();


    int WorldSession::Update(std::chrono::steady_clock::time_point now)
    {
        // Signal this socket is dead and must be removed
        if (m_UnderlyingState == UnderlyingState::CRITICAL_ERROR)
            return -1;

        if (m_UnderlyingState == UnderlyingState::DEFAULT)
        {
            // Startup the socket straight away
            m_UnderlyingState = UnderlyingState::JUST_CONNECTED;
        }
        
        if (m_UnderlyingState == UnderlyingState::JUST_CONNECTED)
        {
            // Update last activity
            m_lastActivity = now;

            // Start the async read loop
            AsyncRead();

            m_UnderlyingState = UnderlyingState::CONNECTED;
        }
        else if (m_UnderlyingState == UnderlyingState::CONNECTED)
        {
            // Check for connected timeout
            if (now - m_lastActivity > std::chrono::milliseconds(Server::Instance().GetSettings().CONNECTED_AND_IDLE_TIMEOUT_MS))
            {
                // Kick this client for inactivity
                LOG_DEBUG("Kicking a client for connected-inactivity.");
                CloseSocket();
            }
        }

        return 0;
    }

    int WorldSession::AsyncReadCallback()
    {
        LOG_DEBUG("WorldSession ReadCallback");

        NetworkMessage& encryptedPacket = m_inBuffer;

        // If we're waiting for the first greetpacket, let's handle it here
        if (m_status == WorldSocketStatus::GATHER_SESSIONKEY)
        {
            // We need to read 128bit greet code first
            // If we don't have the greetcode yet, we'll try again later
            if (encryptedPacket.GetActiveSize() < AES_128_KEY_SIZE)
            {
                AsyncRead(); // queue another Async Read
                return 0;
            }

            // We have the greetcode, let's extract it
            // Copy data from the packet into m_data
            std::copy(encryptedPacket.GetReadPointer(), encryptedPacket.GetReadPointer() + AES_128_KEY_SIZE, m_data.greetCode.begin());
            encryptedPacket.ReadCompleted(AES_128_KEY_SIZE); // Consume the greetcode from the packet buffer

            // Advance phase, we've got the greetcode, now let's fetch it from the db:
            m_status = WorldSocketStatus::GATHER_SESSIONKEY_PENDING;

            auto& dbworker = Server::Instance().GetLoginDBWorker();
            {
                DBRequest req(m_ioContextRef, false);
                req.m_steps.push_back({ static_cast<uint32_t>(LoginDatabaseStatements::SEL_SESSIONKEY_BY_GREETCODE), {mysqlx::bytes(m_data.greetCode.data(), m_data.greetCode.size())}});
                req.m_steps.push_back({ static_cast<uint32_t>(LoginDatabaseStatements::INVALIDATE_GREETCODE), {mysqlx::bytes(m_data.greetCode.data(), m_data.greetCode.size())} });

                // The callback needs to ensure the object still exists, as it may be deleted by the main thread while the dbrequest is being processed
                std::weak_ptr<WorldSession> weakSelf = shared_from_this();
                req.m_callback = [weakSelf](uint32_t ec, std::vector<mysqlx::SqlResult>& res)
                    {
                        if (auto lockedSelf = weakSelf.lock())
                            return lockedSelf->DBCallback_GreetcodeLookup(ec, res);

                        return false; // WorldSession is destroyed (disconnect)
                    };

                req.m_cancelToken = weakSelf;
                dbworker.Enqueue(std::move(req));
            }

            // We cannot decrypt anything until the DB responds, we stop reading for now until DB responds.
            return 0;
        }

        while (encryptedPacket.GetActiveSize())
        {
            // Try to decrypt a whole packet
            int plaintextLen = encryptedPacket.AESDecrypt(m_data.sessionKey.data(), nullptr, 0);
            if (plaintextLen <= 0) // if plaintext is 0, it means the client sent an empty packet, shouldn't ever happen
            {
                if (plaintextLen == -1) // Short receive
                    break;
                
                LOG_WARNING("Decrypt failed for session {} (code {}). Closing.",
                    m_data.accountID, plaintextLen);
                return -1;
            }

            // Get the inner decrypted packet
            m_currentDecryptedPacket.Write(encryptedPacket.GetDecryptedPacketPtr(), plaintextLen);

            // Packet is here the decrpyted [CMD | ...] and it arrived fully
            uint8_t cmd = m_currentDecryptedPacket.GetReadPointer()[0];

            auto it = Handlers.find(cmd);
            if (it == Handlers.end())
            {
                LOG_WARNING("Discarding unknown world packet. CMD: {}", cmd);
                m_currentDecryptedPacket.Clear();
                return -1; // TODO we may want to add some tolerance for unknown packets for the world server
            }

            if (m_status != it->second.status)
            {
                LOG_WARNING("World status mismatch (got {} for cmd {}, expected {}). Closing.",
                    static_cast<int>(m_status), cmd,
                    static_cast<int>(it->second.status));
                return -1;
            }

            uint16_t size = uint16_t(it->second.packetSize);
            if (m_currentDecryptedPacket.GetActiveSize() != size)
                return -1; // Make sure packet's size is in line with what's expected

            try
            {
                // Call the Handler's function and ensure it returns true
                if (!(*this.*it->second.handler)())
                {
                    return -1;
                }
            }
            // Exceptions caught during callback handling must close the socket
            catch (const mysqlx::Error& err) 
            {
                LOG_CRITICAL("Exception caught during callback handling. MySQL Error: {}", err.what());
                return -1;
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

        AsyncRead();
        return 0;
    }

    void WorldSession::AsyncWriteCallback()
    {
        // Update last activity
        m_lastActivity = std::chrono::steady_clock::now();

        if (m_closeAfterSend && m_outQueue.size() == 0)
        {
            LOG_DEBUG("Send Callback called on m_closeAfterSend.");
            CloseSocket();
        }
    }

    bool WorldSession::DBCallback_GreetcodeLookup(uint32_t ec, std::vector<mysqlx::SqlResult>& result)
    {
        if (ec != 0)
        {
            LOG_DEBUG("Greetcode lookup query failed. Dropping socket.");
            CloseSocket();
            return false;
        }

        mysqlx::Row row = result[0].fetchOne();
        if (!row)
        {
            // No session matches this greetcode, client never authenticated or replay
            LOG_INFO("World greetcode not found in active_sessions. Dropping.");
            CloseSocket();
            return false;
        }

        // [userid, sessionKey, starttime, authip]
        m_data.accountID = row[0].get<uint32_t>();

        mysqlx::bytes keyBytes = row[1].get<mysqlx::bytes>();
        if (keyBytes.size() != AES_128_KEY_SIZE)
        {
            LOG_CRITICAL("Stored sessionKey has wrong size ({}).", keyBytes.size());
            CloseSocket();
            return false;
        }
        std::memcpy(m_data.sessionKey.data(), keyBytes.begin(), AES_128_KEY_SIZE);

        // TODO Make sure greetcode is still valid using starttime

        // TODO Make sure the AuthIP that authenticated is the same as the one that connected to the worldserver

        // Now we're ready to receive the encrypted AUTH_SESSION packet, and greetCode is invalidated.
        m_status = WorldSocketStatus::SESSIONKEY_GATHERED;

        // Let's read straight away and restart the AsyncRead loop
        // If there's an error, we need to close the socket here
        int readCall = AsyncReadCallback();

        if (readCall == -1)
        {
            CloseSocket();
            return false;
        }

        return true;
    }

    bool WorldSession::HandleGreetPacket()
    {
        SPacketWorldGreet* pkt = reinterpret_cast<SPacketWorldGreet*>(m_currentDecryptedPacket.GetReadPointer());

        // Get client prefix
        m_data.clientsIVPrefix = pkt->clientsPrefix;

        // Pick our own IV prefix
        m_data.iv.RandomizePrefix();
        while (m_data.iv.prefix == m_data.clientsIVPrefix)
            m_data.iv.RandomizePrefix();

        m_data.iv.ResetCounter();

        m_status = World::WorldSocketStatus::WAITING_FOR_CHALLENGE;

        LOG_CRITICAL("Greet Packet handled! ClientPrefix: {}", m_data.clientsIVPrefix);

        // Send challenge packet to the client

        return true;
    }

}
}
