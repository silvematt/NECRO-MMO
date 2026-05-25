#include "WorldSession.h"
#include "NECROWorld.h"

namespace NECRO
{
namespace World
{

    std::unordered_map<uint16_t, WorldHandler> WorldSession::InitHandlers()
    {
        std::unordered_map<uint16_t, WorldHandler> handlers;

        handlers[static_cast<int>(World::PacketIDs::AUTH_SESSION)] = { NECRO::World::WorldSocketStatus::SESSIONKEY_GATHERED, sizeof(NECRO::World::SPacketWorldGreet) , &HandleGreetPacket };
        handlers[static_cast<int>(World::PacketIDs::CHAR_CREATE_NEW)] = { NECRO::World::WorldSocketStatus::AUTHED, sizeof(NECRO::World::SPacketCreateNewChar) , &Handle_SPacketCreateNewChar };

        return handlers;
    }
    std::unordered_map<uint16_t, WorldHandler> const Handlers = WorldSession::InitHandlers();


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

            auto& dbworker = Server::Instance().GetLoginDBPool();
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
                
                LOG_WARNING("Decrypt failed for session {} (code {}). Closing.", m_data.accountID, plaintextLen);
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
                return -1; // TODO we may want to add some tolerance for unknown packets for the world server
            }

            if (m_status != it->second.status)
            {
                LOG_WARNING("World status mismatch (got {} for cmd {}, expected {}). Closing.",
                    static_cast<int>(m_status), cmd,
                    static_cast<int>(it->second.status));
                return -1;
            }

            // No size check is done here, we know we already already have the full packet the client sent from the AESDecrypt, and we do size and data validation in the Handlers.
            //uint16_t size = static_cast<uint16_t>(it->second.packetSize);
            //if (m_currentDecryptedPacket.GetActiveSize() != size)
            //{
            //    LOG_DEBUG("Discarding client, packet size was {} but active {}", size, m_currentDecryptedPacket.GetActiveSize());
            //    return -1; // Make sure packet's size is in line with what's expected
            //}

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
        if (!IsOpen())
            return false;

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

        // [userid, sessionKey, starttime, authip] TODO ADD USERNAME AS WELL

        // Make sure greetcode is still valid using starttime
        time_t now = time(nullptr);
        time_t requestStartTime = static_cast<time_t>(row[2].get<uint32_t>());

        if (now > requestStartTime + GREETCODE_VALIDITY_TIME_WINDOW_SECONDS)
        {
            LOG_DEBUG("Request is too old, dropping the socket.");
            CloseSocket();
            return false;
        }

        // We could use a machine fingerprint check to compare who made the auth request and who made this request, it may be reduntant security wise 
        // but it could be an early rejection before the AES decrypt + GCM tag verification fails

        m_data.accountID = row[0].get<uint32_t>();

        mysqlx::bytes keyBytes = row[1].get<mysqlx::bytes>();
        if (keyBytes.size() != AES_128_KEY_SIZE)
        {
            LOG_CRITICAL("Stored sessionKey has wrong size ({}).", keyBytes.size());
            CloseSocket();
            return false;
        }
        std::memcpy(m_data.sessionKey.data(), keyBytes.begin(), AES_128_KEY_SIZE);

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

        // Pre-Checks
        // Fixed size differs from what was expected. Can be an hard != because the packet size is fixed
        if (m_currentDecryptedPacket.GetActiveSize() != sizeof(SPacketWorldGreet))
            return false;
        
        // Get client prefix
        m_data.clientsIVPrefix = pkt->clientsPrefix;

        // Pick our own IV prefix
        m_data.iv.RandomizePrefix();
        while (m_data.iv.prefix == m_data.clientsIVPrefix)
            m_data.iv.RandomizePrefix();

        m_data.iv.ResetCounter();

        m_status = World::WorldSocketStatus::SELECTING_CHARACTERS;

        LOG_CRITICAL("Greet Packet handled! Gathering characters list for AccountID: {}", m_data.accountID);

        auto& dbworker = Server::Instance().GetCharactersDBPool();
        {
            DBRequest req(m_ioContextRef, false);
            req.m_steps.push_back({ static_cast<uint32_t>(CharactersDatabaseStatements::CHAR_SEL_ENUM), {m_data.accountID } });

            // The callback needs to ensure the object still exists, as it may be deleted by the main thread while the dbrequest is being processed
            std::weak_ptr<WorldSession> weakSelf = shared_from_this();
            req.m_callback = [weakSelf](uint32_t ec, std::vector<mysqlx::SqlResult>& res)
                {
                    if (auto lockedSelf = weakSelf.lock())
                        return lockedSelf->DBCallback_HandleGreetPacket(ec, res);

                    return false; // WorldSession is destroyed (disconnect)
                };

            req.m_cancelToken = weakSelf;

            if (!dbworker.TryEnqueue(std::move(req)))
                return false;
        }

        return true;
    }

    bool WorldSession::DBCallback_HandleGreetPacket(uint32_t ec, std::vector<mysqlx::SqlResult>& result)
    {
        if (!IsOpen())
            return false;

        if (ec != 0)
        {
            LOG_DEBUG("DBCallback_HandleGreetPacket's query returned an error. There's no way to continue. Dropping socket.");
            CloseSocket();
            return false;
        }

        LOG_DEBUG("Handling DBCallback_AuthLoginProofPacket for user {}!", m_data.accountID);

        // DB callbacks should also update last activity
        m_lastActivity = std::chrono::steady_clock::now();

        // Reply to the client
        Packet packet;
        packet << static_cast<uint16_t>(PacketIDs::ENUM_CHARACTERS);

        mysqlx::Row row = result[0].fetchOne(); //result[0] is result of m_step[0]

        // Check if there's at leat one result
        if (!row)
        {
            LOG_DEBUG("Account {} has no characters.", m_data.accountID);
            packet << static_cast<uint8_t>(WorldResults::NO_CHARACTERS_FOR_THIS_ACCOUNT);
        }
        else
        {
            packet << static_cast<uint8_t>(WorldResults::SUCCESS);

            // Collect all rows first so we can write the count before character data
            std::vector<mysqlx::Row> rows;
            rows.push_back(std::move(row)); // first row already fetched
            while (mysqlx::Row next = result[0].fetchOne())
                rows.push_back(std::move(next));

            // Write size
            // Get all the variable names sizes togheter
            uint16_t namesSize = 0;
            for (mysqlx::Row& charRow : rows)
                namesSize += (charRow[1].get<std::string>()).length();

            // Write number of characters
            packet << static_cast<uint8_t>(rows.size());

            for (mysqlx::Row& charRow : rows)
            {
                std::string characterName = charRow[1].get<std::string>();

                packet << charRow[0].get<uint32_t>();                       // id
                packet << static_cast<uint8_t>(characterName.length());     // characterNameLength
                packet << characterName;                                    // characterName

                packet << static_cast<uint8_t>(charRow[2].get<int>());       // race
                packet << static_cast<uint8_t>(charRow[3].get<int>());       // gameClass
                packet << static_cast<uint8_t>(charRow[4].get<int>());       // gender

                packet << static_cast<uint8_t>(charRow[5].get<int>());       // level
                packet << charRow[6].get<uint32_t>();                        // xp

                packet << static_cast<uint8_t>(charRow[7].get<int>());       // zone
                packet << charRow[8].get<float>();                           // pos_x
                packet << charRow[9].get<float>();                           // pos_y
                packet << charRow[10].get<float>();                          // pos_z
            }

            LOG_DEBUG("Written {} characters for AccountID: {}.", rows.size(), m_data.accountID);
        }

        NetworkMessage m(std::move(packet));
        int encryptRes = m.AESEncrypt(m_data.sessionKey.data(), m_data.iv, nullptr, 0);
        if (encryptRes < 0)
        {
            LOG_ERROR("Failed to encrypt packet, returned {}. Dropping the connection.", encryptRes);
            return false;
        }
        QueuePacket(std::move(m));

        // Client is Authed, he can send select/create characters - if he's not legit, he won't be able to send a coherent packet
        m_status = WorldSocketStatus::AUTHED;

        return true;
    }

    bool WorldSession::Handle_SPacketCreateNewChar()
    {
        m_lastActivity = std::chrono::steady_clock::now();

        SPacketCreateNewChar* pcktData = reinterpret_cast<SPacketCreateNewChar*>(m_currentDecryptedPacket.GetReadPointer());

        // Pre-Checks
        // Checks to always do in the handlers for the world server. 
        // 
        // For fixed packets like the world greet, we can just check against m_currentDecryptedPacket.GetActiveSize() != sizeof(SPacketWorldGreet)
        // 
        // For variable sized packets, thee client gets kicked if:
        // 1. m_currentDecryptedPacket.Size() < sizeof(SPacketWorldGreet) - decrypted packet size
        // 2. m_currentDecryptedPacket.Size() > sizeof(SPacketWorldGreet) + MAX_VARIABLE_FIELD_LENGTH - decrypted packet size
        // 3. pkt.size < sizeof(SPacketWorldGreet)-INITIAL_SIZE - declared packet size
        // 4. pkt.size > sizeof(SPacketWorldGreet)-INITIAL_SIZE + MAX_VARIABLE_FIELD_LENGTH - declared packet size
        // 5. check if client lied: the packet's size: m_currentDecryptedPacket.GetActiveSize() == (sizeof(NECRO::World::SPacketCreateNewChar)) + pcktData->characterNameLength && m_currentDecryptedPacket.GetActiveSize() == pckt.size + INITIAL_SIZE)
        
        // Check for packet size - we check both against m_currentDecryptedPacket.GetActiveSize and client's delcared size
        // 1.
        if (m_currentDecryptedPacket.GetActiveSize() < sizeof(SPacketCreateNewChar))
            return false;

        // 2.
        if (m_currentDecryptedPacket.GetActiveSize() > sizeof(SPacketCreateNewChar) + CHARACTER_MAX_NAME_LENGTH)
            return false;

        // 3. 
        if (pcktData->size < sizeof(SPacketCreateNewChar) - S_PACKET_CREATE_NEW_CHAR_INITIAL_SIZE)
            return false;

        // 4.
        if (pcktData->size > sizeof(SPacketCreateNewChar) - S_PACKET_CREATE_NEW_CHAR_INITIAL_SIZE + CHARACTER_MAX_NAME_LENGTH)
            return false;

        // 5. 
        if (m_currentDecryptedPacket.GetActiveSize() != sizeof(SPacketCreateNewChar) + pcktData->characterNameLength || m_currentDecryptedPacket.GetActiveSize() != pcktData->size + S_PACKET_CREATE_NEW_CHAR_INITIAL_SIZE)
            return false;

        // Check for fields:
        if (pcktData->characterNameLength == 0 || pcktData->characterNameLength > CHARACTER_MAX_NAME_LENGTH)
            return false;

        // Check for input validation
        for (int i = 0; i < pcktData->characterNameLength; i++)
            if (!std::isalnum(static_cast<unsigned char>(pcktData->characterName[i])))
                return false;

        // All good

        std::string charName((char const*)pcktData->characterName, pcktData->characterNameLength);
        LOG_DEBUG("AccountID {} wants to create '{}', name size={}, race={}, class={}, gender={}.", m_data.accountID, charName, pcktData->characterNameLength, pcktData->race, pcktData->charClass, pcktData->gender);

        // Save players selection
        m_data.newCharacterName = charName;
        m_data.newCharacterRace = pcktData->race;
        m_data.newCharacterGender = pcktData->gender;
        m_data.newCharacterClass = pcktData->charClass;

        // Run queries
        auto& dbworker = Server::Instance().GetCharactersDBPool();
        {
            DBRequest req(m_ioContextRef, false);
            req.m_steps.push_back({ static_cast<uint32_t>(CharactersDatabaseStatements::CHAR_SEL_ENUM), {m_data.accountID } }); // check for characters limit on the account
            req.m_steps.push_back({ static_cast<uint32_t>(CharactersDatabaseStatements::CHAR_CHECK_NAME_ALREADY_IN_USE), {m_data.newCharacterName } }); // check for characters limit on the account
            //TODO: queries to check if race,gender,class exist in the DB definitions
            
            // The callback needs to ensure the object still exists, as it may be deleted by the main thread while the dbrequest is being processed
            std::weak_ptr<WorldSession> weakSelf = shared_from_this();
            req.m_callback = [weakSelf](uint32_t ec, std::vector<mysqlx::SqlResult>& res)
                {
                    if (auto lockedSelf = weakSelf.lock())
                        return lockedSelf->DBCallback_HandleCreateNewCharChecks(ec, res);

                    return false; // WorldSession is destroyed (disconnect)
                };

            req.m_cancelToken = weakSelf;

            if (!dbworker.TryEnqueue(std::move(req)))
                return false; // TODO, we could return a SERVER_BUSY and not drop the connection, but just cancel the current action if possible
        }

        return true;
    }

    bool WorldSession::DBCallback_HandleCreateNewCharChecks(uint32_t ec, std::vector<mysqlx::SqlResult>& result)
    {
        if (!IsOpen())
            return false;

        if (ec != 0)
        {
            LOG_DEBUG("DBCallback_HandleCreateNewCharChecks's query returned an error. There's no way to continue. Dropping socket.");
            CloseSocket();
            return false;
        }

        LOG_DEBUG("Handling DBCallback_HandleCreateNewCharChecks for user {}!", m_data.accountID);

        // DB callbacks should also update last activity
        m_lastActivity = std::chrono::steady_clock::now();

        // result[0] - characters number on the account
        // result[1] - is name in use

        // Fail cases
        Packet p;
        p << static_cast<uint16_t>(PacketIDs::CHAR_CREATE_NEW);

        if (result[0].count() >= MAX_CHARACTERS_N_PER_ACCOUNT)
        {
            // Account has all characters
            LOG_DEBUG("Account has all characters!");

            p << static_cast<uint8_t>(WorldResults::CHARACTER_NEW_ACCOUNT_HAS_MAX_CHARACTERS_ALLOWED);
        }
        else  if (result[1].count() > 0)
        {
            // Name already in use
            LOG_DEBUG("Name already in use!");
            p << static_cast<uint8_t>(WorldResults::CHARACTER_NEW_NAME_ALREADY_IN_USE);
        }
        else
        {
            LOG_DEBUG("Checks passed!");

            // Character can be created, run creation query
            // TODO, here we would also have information and validation of the class/race, so that we can set custom start zones / positions / skills / whatever - for now we just hardcode them
            auto& dbworker = Server::Instance().GetCharactersDBPool();
            {
                DBRequest req(m_ioContextRef, false);
                req.m_steps.push_back({ static_cast<uint32_t>(CharactersDatabaseStatements::CHAR_INS_CHARACTER), 
                    {m_data.accountID, m_data.newCharacterName, m_data.newCharacterRace, m_data.newCharacterClass, m_data.newCharacterGender, 1, 0, 0, 0.0f, 0.0f, 0.0f } }); // check for characters limit on the account

                // The callback needs to ensure the object still exists, as it may be deleted by the main thread while the dbrequest is being processed
                std::weak_ptr<WorldSession> weakSelf = shared_from_this();
                req.m_callback = [weakSelf](uint32_t ec, std::vector<mysqlx::SqlResult>& res)
                    {
                        if (auto lockedSelf = weakSelf.lock())
                            return lockedSelf->DBCallback_HandleCreateNewCharFinal(ec, res);

                        return false; // WorldSession is destroyed (disconnect)
                    };

                req.m_cancelToken = weakSelf;

                if (!dbworker.TryEnqueue(std::move(req)))
                {
                    // Also close the socket, returning false won't close the connection
                    // TODO, we could return a SERVER_BUSY and not drop the connection, but just cancel the current action if possible
                    CloseSocket();
                    return false;
                }
            }

            return true;
        }

        // Character couldn't be created
        NetworkMessage m(std::move(p));
        int encryptRes = m.AESEncrypt(m_data.sessionKey.data(), m_data.iv, nullptr, 0);
        if (encryptRes < 0)
        {
            LOG_ERROR("Failed to encrypt packet, returned {}. Dropping the connection.", encryptRes);
            CloseSocket();
            return false;
        }
        QueuePacket(std::move(m));

        return true;
    }

    bool WorldSession::DBCallback_HandleCreateNewCharFinal(uint32_t ec, std::vector<mysqlx::SqlResult>& result)
    {
        if (!IsOpen())
            return false;

        if (ec != 0)
        {
            // TODO, we could return a SERVER_BUSY and not drop the connection, but just cancel the current action if possible
            LOG_DEBUG("DBCallback_HandleCreateNewCharFinal's query returned an error. There's no way to continue. Dropping socket.");
            CloseSocket();
            return false;
        }

        LOG_DEBUG("Handling DBCallback_HandleCreateNewCharFinal for user {}!", m_data.accountID);
        LOG_DEBUG("Character Created!");

        // DB callbacks should also update last activity
        m_lastActivity = std::chrono::steady_clock::now();

        // If ec error check passed, character was created
        Packet p;
        p << static_cast<uint16_t>(PacketIDs::CHAR_CREATE_NEW);
        p << static_cast<uint8_t>(WorldResults::CHARACTER_NEW_SUCCESS);

        NetworkMessage m(std::move(p));
        int encryptRes = m.AESEncrypt(m_data.sessionKey.data(), m_data.iv, nullptr, 0);
        if (encryptRes < 0)
        {
            LOG_ERROR("Failed to encrypt packet, returned {}. Dropping the connection.", encryptRes);
            CloseSocket();
            return false;
        }
        QueuePacket(std::move(m));

        return true;
    }

}
}
