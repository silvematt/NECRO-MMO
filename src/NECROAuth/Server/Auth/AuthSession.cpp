#include "AuthSession.h"
#include "NECROServer.h"
#include "AuthCodes.h"
#include "DBRequest.h"

#include <random>
#include <sstream>
#include <iomanip>
#include <sodium.h>

namespace NECRO
{
namespace Auth
{
    static bool VerifyProofOfWork(const uint8_t* challenge, uint64_t* answer, uint8_t difficulty)
    {
        // TODO we could avoid thread_local, also this is never freed on shutdown
        static thread_local EVP_MD_CTX* ctx = EVP_MD_CTX_new();
        if (!ctx)
            return false;

        uint8_t hash[EVP_MAX_MD_SIZE];
        unsigned int hashLen = 0;

        bool success =
                EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr) == 1 &&
                EVP_DigestUpdate(ctx, challenge, AES_128_KEY_SIZE) == 1 &&
                EVP_DigestUpdate(ctx, answer, sizeof(uint64_t)) == 1 &&
                EVP_DigestFinal_ex(ctx, hash, &hashLen) == 1;

        if (!success || hashLen != SHA256_DIGEST_LENGTH)
            return false;

        return Utility::ProofOfWork_HasLeadingZeroBits(hash, difficulty);
    }

    std::unordered_map<uint8_t, AuthHandler> AuthSession::InitHandlers()
    {
        std::unordered_map<uint8_t, AuthHandler> handlers;

        handlers[static_cast<uint8_t>(PacketIDs::LOGIN_GATHER_INFO)] = { SocketStatus::GATHER_INFO, S_PACKET_AUTH_LOGIN_GATHER_INFO_INITIAL_SIZE, &HandleAuthLoginGatherInfoPacket };
        handlers[static_cast<uint8_t>(PacketIDs::LOGIN_ATTEMPT)] = { SocketStatus::LOGIN_ATTEMPT, S_PACKET_AUTH_LOGIN_PROOF_INITIAL_SIZE, &HandleAuthLoginProofPacket };
        handlers[static_cast<uint8_t>(PacketIDs::LOGIN_GATHER_REALMLIST)] = { SocketStatus::AUTHED, sizeof(SPacketGatherRealmlist), &HandleGatherRealmlistPacket };

        return handlers;
    }
    std::unordered_map<uint8_t, AuthHandler> const Handlers = AuthSession::InitHandlers();


    int AuthSession::Update(std::chrono::steady_clock::time_point now)
    {
        // Signal this socket is dead and must be removed
        if (m_UnderlyingState == UnderlyingState::CRITICAL_ERROR)
            return -1;

        if (m_UnderlyingState == UnderlyingState::DEFAULT)
        {
            // Startup the socket
            if (m_usesTLS)
            {
                // Connected, need to handshake
                m_sslSocket->lowest_layer().set_option(tcp::no_delay(true));
                m_sslSocket->set_verify_mode(boost::asio::ssl::verify_none);

                m_UnderlyingState = UnderlyingState::HANDSHAKING;
                m_handshakeStartTime = now;

                // Start the handshake timeout
                auto self1 = shared_from_this();
                m_handshakeTimeoutTimer.expires_after(std::chrono::milliseconds(Server::Instance().GetSettings().HANDSHAKING_AND_IDLE_TIMEOUT_MS));
                m_handshakeTimeoutTimer.async_wait([this, self1](boost::system::error_code const& ec) { HandshakeTimeoutHandler(ec); });

                auto self2 = shared_from_this();
                m_sslSocket->async_handshake(boost::asio::ssl::stream_base::server,
                    [this, self2](const boost::system::error_code& ec)
                    {
                        if (!ec && !m_handshakeTimedout)
                        {
                            m_UnderlyingState = UnderlyingState::JUST_CONNECTED;

                            m_handshakeTimeoutTimer.cancel();

                            // Now we can start reading/writing, first operation will go in JUST_CONNECTED case
                        }
                        else
                        {
                            if (ec)
                            {
                                LOG_ERROR("Error during handshake: {}", ec.what());
                            }

                            CloseSocket();
                        }
                    });
            }
            return 0;
        }
        else if (m_UnderlyingState == UnderlyingState::JUST_CONNECTED)
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

    // Called to stop this socket if the handshake took more than Server::Instance().GetSettings().HANDSHAKING_AND_IDLE_TIMEOUT_MS
    void AuthSession::HandshakeTimeoutHandler(boost::system::error_code const& ec)
    {
        if (ec)
            return;

        if (m_UnderlyingState == UnderlyingState::HANDSHAKING)
        {
            m_handshakeTimedout = true;
            LOG_DEBUG("Kicking a client for handshake-inactivity.");
            CloseSocket();
        }
    }

    int AuthSession::AsyncReadCallback()
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
                LOG_DEBUG("Discarding the packet CMD: '{}' and disconnecting the client...", cmd);
                packet.Clear();
                return -1;
                // No tolerance for the Auth server. For WorldServer we may have some tolerance for messages that gets corrupted in flight
            }

            // Check if the current cmd matches our state
            if (m_status != it->second.status)
            {
                LOG_WARNING("Status mismatch for user: {}. Status is '{}' but should have been '{}'. Closing the connection...", m_data.username, static_cast<int>(m_status), static_cast<int>(it->second.status));
                
                return -1;
            }

            // Check if the passed packet sizes matches the handler's one, otherwise we're not ready to process this yet
            uint16_t size = static_cast<uint16_t>(it->second.packetSize);
            if (packet.GetActiveSize() < size)
                break;

            // If it's a variable-sized packet, we need to ensure size
            if (cmd == static_cast<int>(PacketIDs::LOGIN_GATHER_INFO))
            {
                SPacketAuthLoginGatherInfo* pcktData = reinterpret_cast<SPacketAuthLoginGatherInfo*>(packet.GetReadPointer());
                size += pcktData->size; // we've read the handler's defined packetSize, so this is safe. Attempt to read the remainder of the packet

                // Check for size - checks for packets that are too big but also packet that are too small
                if (size > S_MAX_ACCEPTED_GATHER_INFO_SIZE || size < sizeof(SPacketAuthLoginGatherInfo))
                    return -1;
            }
            else if (cmd == static_cast<int>(PacketIDs::LOGIN_ATTEMPT))
            {
                SPacketAuthLoginProof* pcktData = reinterpret_cast<SPacketAuthLoginProof*>(packet.GetReadPointer());
                size += pcktData->size; // we've read the handler's defined packetSize, so this is safe. Attempt to read the remainder of the packet

                // Check for size - checks for packets that are too big but also packet that are too small
                if (size > S_MAX_ACCEPTED_AUTH_LOGIN_PROOF_SIZE || size < sizeof(SPacketAuthLoginProof))
                    return - 1;
            }

            // At this point, ensure the read size matches the whole packet size
            if (packet.GetActiveSize() < size)
                break;  // probably a short receive

            // Here we received a whole packet
            if (++m_packetsProcessed > MAX_PACKETS_EXCHANGE_PER_CLIENT)
            {
                // We got more packets than we were anticipating for a login
                LOG_DEBUG("MAX_PACKETS_EXCHANGE_PER_CLIENT reached, kicking the client...");
                packet.Clear();
                return -1;
            }

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

            packet.ReadCompleted(size); // Flag the read as completed, the while will look for remaining packets
        }

        AsyncRead();

        return 0;
    }

    void AuthSession::AsyncWriteCallback()
    {
        // Update last activity
        m_lastActivity = std::chrono::steady_clock::now();

        if (m_closeAfterSend && m_outQueue.size() == 0)
        {
            LOG_DEBUG("Send Callback called on m_closeAfterSend.");
            CloseSocket();
        }
    }

    bool AuthSession::HandleAuthLoginGatherInfoPacket()
    {
        // Update last activity (only during meaningful requests)
        m_lastActivity = std::chrono::steady_clock::now();

        SPacketAuthLoginGatherInfo* pcktData = reinterpret_cast<SPacketAuthLoginGatherInfo*>(m_inBuffer.GetReadPointer());

        // Pre Checks
        // Check for username size
        if (pcktData->usernameSize == 0 || pcktData->usernameSize > Auth::MAX_USERNAME_LENGTH)
            return false;

        // Check if client lied about the packet's size
        if (pcktData->size != (sizeof(NECRO::Auth::SPacketAuthLoginGatherInfo) - 1) - NECRO::Auth::S_PACKET_AUTH_LOGIN_GATHER_INFO_INITIAL_SIZE + pcktData->usernameSize)
            return false;

        // Check for username value (input validation)
        for (int i = 0; i < pcktData->usernameSize; i++)
            if (!std::isalnum(static_cast<unsigned char>(pcktData->username[i])))
                return false;

        // Fill data
        std::string login((char const*)pcktData->username, pcktData->usernameSize);
        m_data.username = login;

        m_data.versionMajor = pcktData->versionMajor;
        m_data.versionMinor = pcktData->versionMinor;
        m_data.versionRevision = pcktData->versionRevision;

        auto& serverSettings = Server::Instance().GetSettings();

        // Check version
        if (m_data.versionMajor == serverSettings.CLIENT_VERSION_MAJOR && m_data.versionMinor == serverSettings.CLIENT_VERSION_MINOR && m_data.versionRevision == serverSettings.CLIENT_VERSION_REVISION)
        {
            LOG_DEBUG("Handling AuthLoginInfo for user: {}", m_data.username);
            m_status = SocketStatus::LOGIN_ATTEMPT; // this will flag this client as someone who already sent a GATHER_INFO, so if the same client sends the same packet again, we'll have a status mismatch

            // Proof of Work generation
            std::array<uint8_t, AES_128_KEY_SIZE> randBytes{};
            if (RAND_bytes(randBytes.data(), AES_128_KEY_SIZE) != 1)
            {
                throw std::runtime_error("Failed to generate Proof of Work challenge!");
                return false;
            }
            uint8_t difficulty = 22; // this can be dynamically adjusted in base of the load

            // Save to authdata
            m_data.challenge = randBytes;
            m_data.difficulty = difficulty;

            // Reply to the client
            Packet packet;
            packet << static_cast<uint8_t>(PacketIDs::LOGIN_GATHER_INFO);
            packet << static_cast<uint8_t>(AuthResults::SUCCESS);
            packet << static_cast<uint16_t>(sizeof(CPacketAuthLoginGatherInfo) - 4);

            // Write challenge
            for (int i = 0; i < AES_128_KEY_SIZE; ++i)
                packet << static_cast<uint8_t>(m_data.challenge[i]);

            packet << static_cast<uint8_t>(m_data.difficulty);

            NetworkMessage m(std::move(packet));
            QueuePacket(std::move(m));
        }
        else
        {
            // Reply to the client that he has the wrong version
            m_closeAfterSend = true; // we want to drop the connection
            Packet packet;
            packet << static_cast<uint8_t>(PacketIDs::LOGIN_GATHER_INFO);
            packet << static_cast<uint8_t>(AuthResults::FAILED_WRONG_CLIENT_VERSION);
            packet << static_cast<uint16_t>(0); // size is empty

            NetworkMessage m(std::move(packet));
            QueuePacket(std::move(m));
        }

        return true;
    }

    bool AuthSession::HandleAuthLoginProofPacket()
    {
        // Update last activity (only during meaningful requests)
        m_lastActivity = std::chrono::steady_clock::now();

        SPacketAuthLoginProof* pcktData = reinterpret_cast<SPacketAuthLoginProof*>(m_inBuffer.GetReadPointer());

        // Pre-checks
        // Check for password size
        if (pcktData->passwordSize == 0 || pcktData->passwordSize > Auth::MAX_PASSWORD_LENGTH)
            return false;

        // Check if client lied about the packet's size
        if (pcktData->size != (sizeof(NECRO::Auth::SPacketAuthLoginProof)-1) - NECRO::Auth::S_PACKET_AUTH_LOGIN_PROOF_INITIAL_SIZE + pcktData->passwordSize)
            return false;

        // Check for password value (input validation)
        for (int i = 0; i < pcktData->passwordSize; i++)
            if (!std::isalnum(static_cast<unsigned char>(pcktData->password[i])))
                return false;

        LOG_DEBUG("Handling AuthLoginProof for user {}", m_data.username);
        m_status = SocketStatus::LOGIN_ATTEMPT_PENDING;

        // Check challenge first
        if (!VerifyProofOfWork(&m_data.challenge[0], &pcktData->answer, m_data.difficulty))
        {
            LOG_DEBUG("VerifyProofOfWork failed for user {}", m_data.username);

            m_closeAfterSend = true;
            Packet reply;
            reply << static_cast<uint8_t>(PacketIDs::LOGIN_ATTEMPT);
            reply << static_cast<uint8_t>(LoginProofResults::FAILED);
            reply << static_cast<uint16_t>(sizeof(CPacketAuthLoginProof) - C_PACKET_AUTH_LOGIN_PROOF_INITIAL_SIZE - AES_128_KEY_SIZE - AES_128_KEY_SIZE); // Adjust the size appropriately
            
            NetworkMessage m(std::move(reply));
            QueuePacket(std::move(m));
            // Don't return false, we're going to close after the packet has been sent
        }
        else
        {
            // Client successfully completed proof of work
            std::string p((char const*)pcktData->password, pcktData->passwordSize);
            m_data.pass = p;
            m_data.randIVPrefix = pcktData->clientsIVRandomPrefix;
            
            // Here we would perform checks such as account exists, banned, suspended, IP locked, region locked, etc.
            auto& dbworker = Server::Instance().GetLoginDBWPool();
            {
                DBRequest req(m_ioContextRef, false);
                req.m_steps.push_back({ static_cast<uint32_t>(LoginDatabaseStatements::CREDENTIALS_CHECK), { m_data.username } });

                // The callback needs to ensure the object still exists, as it may be deleted by the main thread while the dbrequest is being processed
                std::weak_ptr<AuthSession> weakSelf = shared_from_this();
                req.m_callback = [weakSelf](uint32_t ec, std::vector<mysqlx::SqlResult>& res)
                {
                    if (auto lockedSelf = weakSelf.lock())
                        return lockedSelf->DBCallback_AuthLoginProofPacket(ec, res);

                    return false; // AuthSession is destroyed (disconnect)
                };

                req.m_cancelToken = weakSelf;

                if (!dbworker.TryEnqueue(std::move(req)))
                    return false;
            }

            return true;
        }

        return true;
    }

    bool AuthSession::DBCallback_AuthLoginProofPacket(uint32_t ec, std::vector<mysqlx::SqlResult>& result)
    {
        if (!IsOpen())
            return false;

        if (ec != 0)
        {
            LOG_DEBUG("DBCallback_AuthLoginProofPacket's query returned an error. There's no way to continue authentication. Dropping socket.");
            CloseSocket();
            return false;
        }

        LOG_DEBUG("Handling DBCallback_AuthLoginProofPacket for user {}!!", m_data.username);

        // DB callbacks should also update last activity
        m_lastActivity = std::chrono::steady_clock::now();

        mysqlx::Row row = result[0].fetchOne(); //result[0] is result of m_step[0]

        // Check if account exists
        if (!row)
        {
            // We do a fake hash just like the else below, and just call HandlePasswordHashResult(false).
            // This would prevent users enumeration via failed attempts, but would triggers useless crypto_pwhash_str_verify - TODO needs to estimate the performances savings
            LOG_CRITICAL("Account {} doesn't exist.", m_data.username);
            std::string hash = "$argon2id$v=19$m=65536,t=2,p=1$CjmAucKFN9/a9Kfj0bFrKw$WaopYKnajv9K6GRfwo0st3sp9xOCDBWdV51s8N5BAYg"; // TODO store this somewhere instead of allocating it each time?
            std::string pass = "thisisapass";
            std::weak_ptr<AuthSession> weakSelf = shared_from_this();
            Server::Instance().GetCryptoThreads().PostWork([weakSelf, hash, pass]()
            {
                if (auto lockedSelf = weakSelf.lock())
                {
                    bool authenticated = crypto_pwhash_str_verify(hash.data(), pass.data(), pass.size()) == 0;

                    boost::asio::post(lockedSelf->m_ioContextRef, [self = lockedSelf, authenticated]()
                    {
                        if (self->IsOpen())
                            self->HandlePasswordHashResult(authenticated, true);
                    });
                }
            });
        }
        else
        {
            // TODO manage in RAM password properly
            m_data.accountID = row[0].get<uint32_t>();

            LOG_INFO("Account {} has DB AccountID: {}.", m_data.username, m_data.accountID);

            std::string hash = row[1].get<std::string>();
            std::string pass = m_data.pass;

            // Password check
            std::weak_ptr<AuthSession> weakSelf = shared_from_this();
            Server::Instance().GetCryptoThreads().PostWork([weakSelf, hash, pass]()
            {
                if (auto lockedSelf = weakSelf.lock())
                {
                    bool authenticated = crypto_pwhash_str_verify(hash.data(), pass.data(), pass.size()) == 0;

                    boost::asio::post(lockedSelf->m_ioContextRef, [self = lockedSelf, authenticated]()
                    {
                        if (self->IsOpen())
                            self->HandlePasswordHashResult(authenticated, false);
                    });
                }
            });
        }

        return true;
    }

    bool AuthSession::HandlePasswordHashResult(bool authenticated, bool preventLog)
    {
        // Reply to the client
        Packet packet;

        packet << static_cast<uint8_t>(PacketIDs::LOGIN_ATTEMPT);

        // Delete password from memory
        sodium_memzero(m_data.pass.data(), m_data.pass.size());
        m_data.pass.clear();

        auto& dbworker = Server::Instance().GetLoginDBWPool();
        if (!authenticated)
        {
            LOG_INFO("User {} tried to send proof with a wrong password.", this->GetRemoteAddressAndPort());

            // TODO also make this a toggleable in config settings, for now we avoid log only on the fake-hash path
            if (!preventLog)
            {
                // Do an async insert on the DB worker to log that his IP tried to login with a wrong password
                {
                    DBRequest req(m_ioContextRef, true);
                    req.m_steps.push_back({ static_cast<uint32_t>(LoginDatabaseStatements::INS_LOG_WRONG_PASSWORD), {this->GetRemoteAddressAndPort(), m_data.username, "WRONG_PASSWORD"} });
                    dbworker.TryEnqueue(std::move(req));
                }
            }

            m_closeAfterSend = true;
            packet << static_cast<uint8_t>(LoginProofResults::FAILED);
            packet << static_cast<uint16_t>(0); // empty content
        }
        else
        {
            // Continue login
            m_status = SocketStatus::AUTHED;

            packet << static_cast<uint8_t>(LoginProofResults::SUCCESS);
            packet << static_cast<uint16_t>(sizeof(CPacketAuthLoginProof) - C_PACKET_AUTH_LOGIN_PROOF_INITIAL_SIZE); // Adjust the size appropriately, here we send the key

            // Calculate this side's IV, making sure it's different from the client's
            m_data.iv.RandomizePrefix();
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
                packet << m_data.sessionKey[i];

            // Create a new greetcode (RAND_bytes). Greetcode is appended with the first packet the client sends to the server, so the server can understand who's he talking to. ONE USE! Server will clear it after first usage
            m_data.greetCode = AES::GenerateSessionKey();

            // Delete every previous sessions (if any) of this user, the game server will notice the new connection and kick him the previous client from the game
            // This is a transaction
            {
                DBRequest req(m_ioContextRef, true);
                req.m_steps.push_back({ static_cast<uint32_t>(LoginDatabaseStatements::DEL_PREV_SESSIONS),  {m_data.accountID} });
                req.m_steps.push_back({ static_cast<uint32_t>(LoginDatabaseStatements::INS_NEW_SESSION),    {m_data.accountID, mysqlx::bytes(m_data.sessionKey.data(), m_data.sessionKey.size()), this->GetRemoteAddress(), mysqlx::bytes(m_data.greetCode.data(), m_data.greetCode.size())} });
                req.m_steps.push_back({ static_cast<uint32_t>(LoginDatabaseStatements::UPD_ON_LOGIN),       {1, Utility::time_stamp(), m_data.accountID} });

                // Handle failure, this is posted on this io_context from the cryptoThreads so return false won't close the socket
                if (!dbworker.TryEnqueue(std::move(req)))
                {
                    LOG_DEBUG("DBWorker was full after password hash verification for {}.", m_data.username);
                    CloseSocket();
                    return false;
                }
            }

            // Write the greetcode to the packet
            for (int i = 0; i < AES_128_KEY_SIZE; ++i)
                packet << m_data.greetCode[i];
        }

        NetworkMessage m(std::move(packet));
        QueuePacket(std::move(m));

        return true;
    }

    bool AuthSession::HandleGatherRealmlistPacket()
    {
        // Update activity and Status
        m_lastActivity = std::chrono::steady_clock::now();
        m_status = SocketStatus::GATHER_REALMLIST_PENDING;

        std::vector<Realm> realms = RealmList::Instance().GetRealmList();

        Packet p;
        p << static_cast<uint8_t>(Auth::PacketIDs::LOGIN_GATHER_REALMLIST);
        p << static_cast<uint8_t>(Auth::AuthResults::SUCCESS);

        Packet payload; // CRealmData bytes[];
        uint32_t realmCount = 0;

        // Write Realm
        for (size_t i = 0; i < realms.size(); i++)
        {
            if (realmCount > MAX_REALMS_N)
                break;

            uint8_t ipAddr[4];
            if (inet_pton(AF_INET, realms[i].ip.c_str(), ipAddr) != 1)
            {
                LOG_ERROR("Invalid IPv4 for realm '{}', skipping it.", realms[i].name);
                continue;
            }

            if (realms[i].name.size() > REALM_MAX_NAME_SIZE)
            {
                LOG_ERROR("Realm name too long for '{}', skipping it.", realms[i].name);
                continue;
            }

            payload << static_cast<uint32_t>(realms[i].ID);
            payload << realms[i].status;
            payload.Append(ipAddr, 4);
            payload << htons(realms[i].port);
            payload << static_cast<uint8_t>(realms[i].name.size());
            payload << realms[i].name;

            realmCount++;
        }

        // This is the final packet, set close after send
        m_closeAfterSend = true;

        // Total Packet Size
        p << static_cast<uint16_t>(4 + payload.Size()); // +4 is for the realm count that gets written before the payload
        p << realmCount;
        p.Append(payload.GetContent(), payload.Size());

        NetworkMessage m(std::move(p));
        QueuePacket(std::move(m));

        LOG_OK("HandleGatherRealmlistPacket: sent {} realm(s)", realmCount);
        return true;
    }
}
}
