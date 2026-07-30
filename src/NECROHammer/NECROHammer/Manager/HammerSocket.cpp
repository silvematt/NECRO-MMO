#include "HammerSocket.h"

#include "FileLogger.h"
#include "ConsoleLogger.h"

#include "AuthCodes.h"

#include <iomanip>

namespace NECRO
{
namespace Hammer
{
    std::unordered_map<uint8_t, HammerHandler> HammerSocket::InitHandlers()
    {
        std::unordered_map<uint8_t, HammerHandler> handlers;

        // fill
        handlers[static_cast<int>(NECRO::Auth::PacketIDs::LOGIN_GATHER_INFO)] = { NECRO::Auth::SocketStatus::GATHER_INFO, sizeof(NECRO::Auth::CPacketAuthLoginGatherInfo) , &HandlePacketAuthLoginGatherInfoResponse };
        handlers[static_cast<int>(NECRO::Auth::PacketIDs::LOGIN_ATTEMPT)] = { NECRO::Auth::SocketStatus::LOGIN_ATTEMPT, NECRO::Auth::C_PACKET_AUTH_LOGIN_PROOF_INITIAL_SIZE , &HandlePacketAuthLoginProofResponse };
        handlers[static_cast<int>(NECRO::Auth::PacketIDs::LOGIN_GATHER_REALMLIST)] = { NECRO::Auth::SocketStatus::AUTHED, NECRO::Auth::C_PACKET_GATHER_REALMLIST_INITIAL_SIZE , &HandlePacketGatherRealmsResponse };

        return handlers;
    }
    std::unordered_map<uint8_t, HammerHandler> const Handlers = HammerSocket::InitHandlers();

	int HammerSocket::Update(std::chrono::steady_clock::time_point now)
	{
		// Signal this socket is dead and must be removed
		if (m_UnderlyingState == UnderlyingState::CRITICAL_ERROR)
			return -1;

		if (m_UnderlyingState == UnderlyingState::DEFAULT)
		{
			// Startup the socket
			Resolve(m_remoteIp, m_remotePort);
			return 0;
		}
		else if (m_UnderlyingState == UnderlyingState::JUST_CONNECTED)
		{
			// Start the async read loop
			AsyncRead();
            
            // Write the first packet
            Packet greetPacket;
            uint8_t usernameLenght = static_cast<uint8_t>(m_data.username.size());;

            greetPacket << static_cast<uint8_t>(NECRO::Auth::PacketIDs::LOGIN_GATHER_INFO);
            greetPacket << static_cast<uint8_t>(NECRO::Auth::AuthResults::SUCCESS);
            greetPacket << static_cast<uint16_t>((sizeof(NECRO::Auth::SPacketAuthLoginGatherInfo)-1) - NECRO::Auth::S_PACKET_AUTH_LOGIN_GATHER_INFO_INITIAL_SIZE + usernameLenght); // this means that after having read the first PACKET_AUTH_LOGIN_GATHER_INFO_INITIAL_SIZE bytes, the server will have to wait for sizeof(PacketAuthLoginGatherInfo) - PACKET_AUTH_LOGIN_GATHER_INFO_INITIAL_SIZE + usernameLenght in order to correctly read this packet

            greetPacket << static_cast<uint8_t>(1); // versionMajor
            greetPacket << static_cast<uint8_t>(0); // versionMinor
            greetPacket << static_cast<uint8_t>(0); // versionRevision

            greetPacket << usernameLenght;
            greetPacket << m_data.username; // string is and should be without null terminator!

            NetworkMessage message(std::move(greetPacket));
            QueuePacket(std::move(message));

            m_UnderlyingState = UnderlyingState::CONNECTED;
		}
		
		return 0;
	}

	int HammerSocket::AsyncReadCallback()
	{
		// Process the data that just came in
		LOG_DEBUG("AsyncReadCallback!");

        NetworkMessage& packet = m_inBuffer;

        while (packet.GetActiveSize())
        {
            uint8_t cmd = packet.GetReadPointer()[0]; // read first byte

            auto it = Handlers.find(cmd);
            if (it == Handlers.end())
            {
                LOG_WARNING("Discarding packet.");

                // Discard packet, nothing we should handle
                packet.Clear();
                break;
            }

            // Check if the current cmd matches our state
            if (m_status != it->second.status)
            {
                LOG_WARNING("Status mismatch. For packet cmd: '{}' Status is: '{}' but should have been '{}'. Closing the connection.", cmd, static_cast<int>(m_status), static_cast<int>(it->second.status));

                CloseSocket();
                return -1;
            }

            // Check if the passed packet sizes matches the handler's one, otherwise we're not ready to process this yet
            uint16_t size = static_cast<uint16_t>(it->second.packetSize);
            if (packet.GetActiveSize() < size)
                break;

            // If it's a variable-sized packet, we need to ensure size
            if (cmd == static_cast<int>(NECRO::Auth::PacketIDs::LOGIN_ATTEMPT))
            {
                NECRO::Auth::CPacketAuthLoginProof* pcktData = reinterpret_cast<NECRO::Auth::CPacketAuthLoginProof*>(packet.GetReadPointer());
                size += pcktData->size; // we've read the handler's defined packetSize, so this is safe. Attempt to read the remainder of the packet

                // Check for size
                if (size > sizeof(NECRO::Auth::CPacketAuthLoginProof))
                {
                    CloseSocket();
                    return -1;
                }
            }
            else if (cmd == static_cast<int>(NECRO::Auth::PacketIDs::LOGIN_GATHER_REALMLIST))
            {
                NECRO::Auth::CPacketGatherRealmlist* pcktData = reinterpret_cast<NECRO::Auth::CPacketGatherRealmlist*>(packet.GetReadPointer());
                size += pcktData->size; // we've read the handler's defined packetSize, so this is safe. Attempt to read the remainder of the packet
            }

            // At this point, ensure the read size matches the whole packet size
            if (packet.GetActiveSize() < size)
                break;  // probably a short receive

            // Call the Handler's function and ensure it returns true
            if (!(*this.*it->second.handler)())
            {
                CloseSocket();
                return-1;
            }

            packet.ReadCompleted(size); // Flag the read as completed, the while will look for remaining packets
        }

		// Start another async read
		AsyncRead();

		return 0;
	}

	void HammerSocket::AsyncWriteCallback()
	{

	}

    bool HammerSocket::HandlePacketAuthLoginGatherInfoResponse()
    {
        NECRO::Auth::CPacketAuthLoginGatherInfo* pckData = reinterpret_cast<NECRO::Auth::CPacketAuthLoginGatherInfo*>(m_inBuffer.GetBasePointer());

        if (pckData->error == static_cast<int>(NECRO::Auth::AuthResults::SUCCESS))
        {
            // Solve the puzzle - this should be done by a background thread to not block the UI
            bool solved = false;
            uint64_t counter = 0;
            uint8_t hash[SHA256_DIGEST_LENGTH];
            // hash = buffer(challenge+counter)
            std::array<uint8_t, AES_128_KEY_SIZE + sizeof(counter)> buffer;
            while (!solved)
            {
                // Append both the challenge and the counter to the buffer
                std::memcpy(buffer.data(), &pckData->challenge, AES_128_KEY_SIZE);
                std::memcpy(buffer.data() + AES_128_KEY_SIZE, &counter, sizeof(counter));

                // Compute SHA256, non EVP way is fine for now - TODO maybe in the future
                SHA256(buffer.data(), AES_128_KEY_SIZE + sizeof(counter), hash);
                solved = Utility::ProofOfWork_HasLeadingZeroBits(hash, pckData->difficulty);

                if (!solved)
                    counter++;
            }

            m_status = NECRO::Auth::SocketStatus::LOGIN_ATTEMPT;

            uint8_t passwordLength = static_cast<uint8_t>(m_data.password.size());;

            // Send SPacketAuthLoginProof
            Packet packet;

            // Send the header
            packet << static_cast<uint8_t>(NECRO::Auth::PacketIDs::LOGIN_ATTEMPT);
            packet << static_cast<uint8_t>(NECRO::Auth::LoginProofResults::SUCCESS);
            packet << static_cast<uint16_t>((sizeof(NECRO::Auth::SPacketAuthLoginProof) - 1) - NECRO::Auth::S_PACKET_AUTH_LOGIN_PROOF_INITIAL_SIZE + passwordLength); // this means that after having read the first S_PACKET_AUTH_LOGIN_PROOF_INITIAL_SIZE bytes, the server will have to wait for sizeof(SPacketAuthLoginProof) - PACKET_AUTH_LOGIN_PROOF_INITIAL_SIZE + passwordLength in order to correctly read this packet

            // Send the PoW answer
            packet << counter;

            // Randomize and send the prefix
            m_data.iv.RandomizePrefix();
            m_data.iv.ResetCounter();

            packet << static_cast<uint32_t>(m_data.iv.prefix);

            packet << passwordLength;
            packet << m_data.password; // string is and should be without null terminator!

            m_data.password.clear(); // clear the password from memory after having used it TODO sodiumzero

            std::cout << "My IV Prefix: " << m_data.iv.prefix << std::endl;

            NetworkMessage m(std::move(packet));
            QueuePacket(std::move(m));
        }
        else if (pckData->error == static_cast<int>(NECRO::Auth::AuthResults::FAILED_UNKNOWN_ACCOUNT))
        {
            LOG_ERROR("Authentication failed, username does not exist.");
            return false;
        }
        else if (pckData->error == static_cast<int>(NECRO::Auth::AuthResults::FAILED_WRONG_CLIENT_VERSION))
        {
            LOG_ERROR("Authentication failed, invalid client version.");
            return false;
        }
        else
        {
            LOG_ERROR("Authentication failed, server hasn't returned AuthResults::AUTH_SUCCESS.");
            return false;
        }

        return true;
    }

    bool HammerSocket::HandlePacketAuthLoginProofResponse()
    {
        NECRO::Auth::CPacketAuthLoginProof* pckData = reinterpret_cast<NECRO::Auth::CPacketAuthLoginProof*>(m_inBuffer.GetBasePointer());

        if (pckData->error == static_cast<int>(NECRO::Auth::LoginProofResults::SUCCESS))
        {
            // Continue authentication
            m_status = NECRO::Auth::SocketStatus::AUTHED;

            // Save the session key in the netManager data
            std::copy(std::begin(pckData->sessionKey), std::end(pckData->sessionKey), std::begin(m_data.sessionKey));

            // Convert sessionKey to hex string in order to print it
            std::ostringstream sessionStrStream;
            for (int i = 0; i < AES_128_KEY_SIZE; ++i)
            {
                sessionStrStream << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(m_data.sessionKey[i]);
            }
            std::string sessionStr = sessionStrStream.str();

            // Save the greetcode in the netmanager data
            std::copy(std::begin(pckData->greetcode), std::end(pckData->greetcode), std::begin(m_data.greetcode));

            // Convert greetcode to hex string in order to print it
            std::ostringstream greetCodeStrStream;
            for (int i = 0; i < AES_128_KEY_SIZE; ++i)
            {
                greetCodeStrStream << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(m_data.greetcode[i]);
            }
            std::string greetStr = greetCodeStrStream.str();

            LOG_DEBUG("My session key is: {}", sessionStr);
            LOG_DEBUG("Greetcode is : {}", greetStr);

            // Auth completed, gather the realms now
            LOG_DEBUG("Authentication completed! Gathering Realms...");
            m_status = Auth::SocketStatus::AUTHED;

            // Send gather request
            Packet p;
            p << static_cast<uint8_t>(Auth::PacketIDs::LOGIN_GATHER_REALMLIST);
            p << static_cast<uint8_t>(Auth::AuthResults::SUCCESS);

            NetworkMessage m(std::move(p));
            QueuePacket(std::move(m));
        }
        else //  (pckData->error == LoginProofResults::LOGIN_FAILED)
        {
            LOG_ERROR("Authentication failed. Server returned LoginProofResults::LOGIN_FAILED.");
            return false;
        }

        return true;
    }

    bool HammerSocket::HandlePacketGatherRealmsResponse()
    {
        NECRO::Auth::CPacketGatherRealmlist* pcktData = reinterpret_cast<NECRO::Auth::CPacketGatherRealmlist*>(m_inBuffer.GetBasePointer());

        if (pcktData->error == static_cast<int>(NECRO::Auth::LoginProofResults::SUCCESS))
        {
            m_status = NECRO::Auth::SocketStatus::GATHER_REALMLIST_PENDING;

            uint8_t numRealms = pcktData->numOfRealms;

            if (numRealms > NECRO::Auth::MAX_REALMS_N)
            {
                LOG_ERROR("Server returned an error while gathering realms.");
                return false;
            }

            // Cursor to read realms bytes
            uint8_t* cursor = reinterpret_cast<uint8_t*>(pcktData->bytes);

            std::vector<Realm>& realmlist = m_data.realmlist;
            realmlist.clear();

            for (size_t i = 0; i < numRealms; i++)
            {
                NECRO::Auth::RealmDataOnWire* realmData = reinterpret_cast<NECRO::Auth::RealmDataOnWire*>(cursor);

                Realm entry;
                entry.ID = realmData->id;
                entry.status = realmData->status;

                // Convert the 4 bytes of the ipAddress to a string
                char ipBuf[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, realmData->ipAddress, ipBuf, INET_ADDRSTRLEN);
                entry.ip = ipBuf;

                entry.port = ntohs(realmData->port);
                entry.name = std::string(reinterpret_cast<char*>(realmData->name), realmData->nameSize);

                LOG_INFO("Realm: ID={}, Name='{}', Address={}:{}, Status={}", entry.ID, entry.name, entry.ip, entry.port, entry.status);

                realmlist.push_back(std::move(entry));

                // Advance cursor past the fixed portion of CRealmData (minus the flexible name[1]) + the actual name size
                cursor += (sizeof(NECRO::Auth::RealmDataOnWire) - 1) + realmData->nameSize;
            }

            LOG_OK("Received {} realm(s).", realmlist.size());

            // Close connection to auth server
            LOG_DEBUG("Authentication completed! Closing Auth Socket...");
            CloseSocket(); // TODO handle TLS shutdown and shutdhown gracefully

            // We're now ready to connect to the game server
            return true;
        }
        else
        {
            LOG_ERROR("Server returned an error while gathering realms.");
            return false;
        }
    }
}
}
