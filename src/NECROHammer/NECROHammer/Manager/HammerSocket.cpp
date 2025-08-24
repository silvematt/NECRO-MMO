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

        return handlers;
    }
    std::unordered_map<uint8_t, HammerHandler> const Handlers = HammerSocket::InitHandlers();

	int HammerSocket::Update()
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

			greetPacket << uint8_t(NECRO::Auth::PacketIDs::LOGIN_GATHER_INFO);
			greetPacket << uint8_t(NECRO::Auth::AuthResults::SUCCESS);
			greetPacket << uint16_t(sizeof(NECRO::Auth::SPacketAuthLoginGatherInfo) - NECRO::Auth::S_PACKET_AUTH_LOGIN_GATHER_INFO_INITIAL_SIZE + usernameLenght - 1); // this means that after having read the first PACKET_AUTH_LOGIN_GATHER_INFO_INITIAL_SIZE bytes, the server will have to wait for sizeof(PacketAuthLoginGatherInfo) - PACKET_AUTH_LOGIN_GATHER_INFO_INITIAL_SIZE + usernameLenght-1 bytes in order to correctly read this packet

			greetPacket << uint8_t(1); // versionMajor
			greetPacket << uint8_t(0); // versionMinor
			greetPacket << uint8_t(0); // versionRevision

			greetPacket << usernameLenght;
			greetPacket << m_data.username; // string is and should be without null terminator!

			NetworkMessage message(greetPacket);
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
                LOG_WARNING("Status mismatch. Status is: '{}' but should have been '{}'. Closing the connection.", static_cast<int>(m_status), static_cast<int>(it->second.status));

                CloseSocket();
                return -1;
            }

            // Check if the passed packet sizes matches the handler's one, otherwise we're not ready to process this yet
            uint16_t size = uint16_t(it->second.packetSize);
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
            // Continue authentication
            m_status = NECRO::Auth::SocketStatus::LOGIN_ATTEMPT;

            uint8_t passwordLength = static_cast<uint8_t>(m_data.password.size());;

            // Here you would send login proof to the server, after having received hashes in the CPacketAuthLoginGatherInfo packet above
            // Send the random IV prefix so the server can make sure it's not the same as the client
            Packet packet;

            packet << uint8_t(NECRO::Auth::PacketIDs::LOGIN_ATTEMPT);
            packet << uint8_t(NECRO::Auth::LoginProofResults::SUCCESS);
            packet << uint16_t(sizeof(NECRO::Auth::SPacketAuthLoginProof) - NECRO::Auth::S_PACKET_AUTH_LOGIN_PROOF_INITIAL_SIZE + passwordLength - 1); // this means that after having read the first S_PACKET_AUTH_LOGIN_PROOF_INITIAL_SIZE bytes, the server will have to wait for sizeof(SPacketAuthLoginProof) - PACKET_AUTH_LOGIN_PROOF_INITIAL_SIZE + passwordLength-1 bytes in order to correctly read this packet

            // Randomize and send the prefix
            m_data.iv.RandomizePrefix();
            m_data.iv.ResetCounter();

            packet << uint32_t(m_data.iv.prefix);

            packet << passwordLength;
            packet << m_data.password; // string is and should be without null terminator!

            m_data.password.clear(); // clear the password from memory after having used it

            std::cout << "My IV Prefix: " << m_data.iv.prefix << std::endl;

            NetworkMessage m(packet);
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

            // Save the session key in this session data
            std::copy(std::begin(pckData->sessionKey), std::end(pckData->sessionKey), std::begin(m_data.sessionKey));

            // Convert sessionKey to hex string in order to print it
            std::ostringstream sessionStrStream;
            for (int i = 0; i < AES_128_KEY_SIZE; ++i)
            {
                sessionStrStream << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(m_data.sessionKey[i]);
            }
            std::string sessionStr = sessionStrStream.str();

            // Save the greetcode in this session data
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

            // Close connection to auth server
            LOG_OK("Authentication completed! Closing Auth Socket...");
            CloseSocket();
        }
        else //  (pckData->error == LoginProofResults::LOGIN_FAILED)
        {
            LOG_ERROR("Authentication failed. Server returned LoginProofResults::LOGIN_FAILED.");
            return false;
        }

        return true;
    }

}
}
