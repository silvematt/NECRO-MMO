#include "AuthSession.h"
#include "ConsoleLogger.h"
#include "FileLogger.h"
#include "NECROEngine.h"

#include "AuthCodes.h"

#include <sstream>
#include <iomanip>

#include "AES.h"

namespace NECRO
{
namespace Client
{
    std::unordered_map<uint8_t, AuthHandler> AuthSession::InitHandlers()
    {
        std::unordered_map<uint8_t, AuthHandler> handlers;

        // fill
        handlers[static_cast<int>(NECRO::Auth::PacketIDs::LOGIN_GATHER_INFO)] = { NECRO::Auth::SocketStatus::GATHER_INFO, sizeof(NECRO::Auth::CPacketAuthLoginGatherInfo) , &HandlePacketAuthLoginGatherInfoResponse };
        handlers[static_cast<int>(NECRO::Auth::PacketIDs::LOGIN_ATTEMPT)] = {NECRO::Auth::SocketStatus::LOGIN_ATTEMPT, C_PACKET_AUTH_LOGIN_PROOF_INITIAL_SIZE , &HandlePacketAuthLoginProofResponse};

        return handlers;
    }
    std::unordered_map<uint8_t, AuthHandler> const Handlers = AuthSession::InitHandlers();


    void AuthSession::OnConnectedCallback()
    {
        AuthManager& netManager = engine.GetAuthManager();
        Console& c = engine.GetConsole();

        // Get local IP address
        sockaddr_in local_addr{};
        socklen_t addr_len = sizeof(local_addr);
        if (getsockname(GetSocketFD(), (sockaddr*)&local_addr, &addr_len) == 0)
        {
            char local_ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &local_addr.sin_addr, local_ip, INET_ADDRSTRLEN);

            std::string ipStr(local_ip);
            c.Log("My Local IP: " + ipStr);
            netManager.SetAuthDataIpAddress(ipStr);
        }

        // Send greet packet
        Packet greetPacket;
        uint8_t usernameLenght = static_cast<uint8_t>(netManager.GetData().username.size());;

        greetPacket << uint8_t(NECRO::Auth::PacketIDs::LOGIN_GATHER_INFO);
        greetPacket << uint8_t(NECRO::Auth::AuthResults::SUCCESS);
        greetPacket << uint16_t(sizeof(NECRO::Auth::SPacketAuthLoginGatherInfo) - S_PACKET_AUTH_LOGIN_GATHER_INFO_INITIAL_SIZE + usernameLenght - 1); // this means that after having read the first PACKET_AUTH_LOGIN_GATHER_INFO_INITIAL_SIZE bytes, the server will have to wait for sizeof(PacketAuthLoginGatherInfo) - PACKET_AUTH_LOGIN_GATHER_INFO_INITIAL_SIZE + usernameLenght-1 bytes in order to correctly read this packet

        greetPacket << CLIENT_VERSION_MAJOR;
        greetPacket << CLIENT_VERSION_MINOR;
        greetPacket << CLIENT_VERSION_REVISION;

        greetPacket << usernameLenght;
        greetPacket << netManager.GetData().username; // string is and should be without null terminator!

        NetworkMessage message(greetPacket);
        QueuePacket(std::move(message));
        //Send(); packets are sent by checking POLLOUT events in the socket, and we check for POLLOUT events only if there are packets written in the outQueue
    }

    void AuthSession::ReadCallback()
    {
        LOG_OK("AuthSession ReadCallback");

        NetworkMessage& packet = inBuffer;

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
            if (status != it->second.status)
            {
                LOG_WARNING("Status mismatch. Status is: '" + std::to_string(static_cast<int>(status)) + "' but should have been '" + std::to_string(static_cast<int>(it->second.status)) + "'. Closing the connection.");

                Shutdown();
                Close();
                return;
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

    bool AuthSession::HandlePacketAuthLoginGatherInfoResponse()
    {
        Console& c = engine.GetConsole();
        NECRO::Auth::CPacketAuthLoginGatherInfo* pckData = reinterpret_cast<NECRO::Auth::CPacketAuthLoginGatherInfo*>(inBuffer.GetBasePointer());
        AuthManager& net = engine.GetAuthManager();

        if (pckData->error == static_cast<int>(NECRO::Auth::AuthResults::SUCCESS))
        {
            // Continue authentication
            c.Log("Gather info succeded...");
            status = NECRO::Auth::SocketStatus::LOGIN_ATTEMPT;

            uint8_t passwordLength = static_cast<uint8_t>(net.GetData().password.size());;

            // Here you would send login proof to the server, after having received hashes in the CPacketAuthLoginGatherInfo packet above
            // Send the random IV prefix so the server can make sure it's not the same as the client
            Packet packet;

            packet << uint8_t(NECRO::Auth::PacketIDs::LOGIN_ATTEMPT);
            packet << uint8_t(NECRO::Auth::LoginProofResults::SUCCESS);
            packet << uint16_t(sizeof(NECRO::Auth::SPacketAuthLoginProof) - S_PACKET_AUTH_LOGIN_PROOF_INITIAL_SIZE + passwordLength - 1); // this means that after having read the first S_PACKET_AUTH_LOGIN_PROOF_INITIAL_SIZE bytes, the server will have to wait for sizeof(SPacketAuthLoginProof) - PACKET_AUTH_LOGIN_PROOF_INITIAL_SIZE + passwordLength-1 bytes in order to correctly read this packet

            // Randomize and send the prefix
            net.GetData().iv.RandomizePrefix();
            net.GetData().iv.ResetCounter();

            packet << uint32_t(net.GetData().iv.prefix);

            packet << passwordLength;
            packet << net.GetData().password; // string is and should be without null terminator!

            net.GetData().password.clear(); // clear the password from memory after having used it

            std::cout << "My IV Prefix: " << net.GetData().iv.prefix << std::endl;

            NetworkMessage m(packet);
            QueuePacket(std::move(m));
            //Send(); packets are sent by checking POLLOUT events in the socket, and we check for POLLOUT events only if there are packets written in the outQueue

        }
        else if (pckData->error == static_cast<int>(NECRO::Auth::AuthResults::FAILED_USERNAME_IN_USE))
        {
            LOG_ERROR("Authentication failed, username is already in use.");
            c.Log("Authentication failed. Username is already in use.");
            return false;
        }
        else if (pckData->error == static_cast<int>(NECRO::Auth::AuthResults::FAILED_UNKNOWN_ACCOUNT))
        {
            LOG_ERROR("Authentication failed, username does not exist.");
            c.Log("Authentication failed, username does not exist.");
            return false;
        }
        else if (pckData->error == static_cast<int>(NECRO::Auth::AuthResults::FAILED_WRONG_CLIENT_VERSION))
        {
            LOG_ERROR("Authentication failed, invalid client version.");
            c.Log("Authentication failed, invalid client version.");
            return false;
        }
        else
        {
            LOG_ERROR("Authentication failed, server hasn't returned AuthResults::AUTH_SUCCESS.");
            c.Log("Authentication failed.");
            return false;
        }

        return true;
    }

    bool AuthSession::HandlePacketAuthLoginProofResponse()
    {
        AuthManager& netManager = engine.GetAuthManager();

        Console& c = engine.GetConsole();
        NECRO::Auth::CPacketAuthLoginProof* pckData = reinterpret_cast<NECRO::Auth::CPacketAuthLoginProof*>(inBuffer.GetBasePointer());

        if (pckData->error == static_cast<int>(NECRO::Auth::LoginProofResults::SUCCESS))
        {
            // Continue authentication
            c.Log("Authentication succeeded.");
            status = NECRO::Auth::SocketStatus::AUTHED;

            // Save the session key in the netManager data
            std::copy(std::begin(pckData->sessionKey), std::end(pckData->sessionKey), std::begin(netManager.GetData().sessionKey));

            // Convert sessionKey to hex string in order to print it
            std::ostringstream sessionStrStream;
            for (int i = 0; i < AES_128_KEY_SIZE; ++i)
            {
                sessionStrStream << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(netManager.GetData().sessionKey[i]);
            }
            std::string sessionStr = sessionStrStream.str();

            // Save the greetcode in the netmanager data
            std::copy(std::begin(pckData->greetcode), std::end(pckData->greetcode), std::begin(netManager.GetData().greetcode));

            // Convert greetcode to hex string in order to print it
            std::ostringstream greetCodeStrStream;
            for (int i = 0; i < AES_128_KEY_SIZE; ++i)
            {
                greetCodeStrStream << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(netManager.GetData().greetcode[i]);
            }
            std::string greetStr = greetCodeStrStream.str();

            LOG_DEBUG("My session key is: " + sessionStr);
            LOG_DEBUG("Greetcode is : " + greetStr);

            // Close connection to auth server
            LOG_DEBUG("Authentication completed! Closing Auth Socket...");
            Close();

            // We're now ready to connect to the game server
            // This packet (AuthLoginProofResponse) could also contain the realms list
            netManager.OnAuthenticationCompleted();
        }
        else //  (pckData->error == LoginProofResults::LOGIN_FAILED)
        {
            LOG_ERROR("Authentication failed. Server returned LoginProofResults::LOGIN_FAILED.");
            c.Log("Authentication failed.");
            return false;
        }

        return true;
    }

}
}
