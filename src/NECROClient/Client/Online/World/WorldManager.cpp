#include "WorldManager.h"
#include "NECROEngine.h"

#include <vector>

namespace NECRO
{
namespace Client
{
    int WorldManager::Init()
    {
        // SocketUtility and OpenSSL are already initialized by the AuthManager
        CreateWorldSocket();
        return 0;
    }

    void WorldManager::CreateWorldSocket()
    {
        m_isConnecting = false;

        m_worldSocket = std::make_unique<WorldSession>(SocketAddressesFamily::INET);

        int flag = 1;
        m_worldSocket->SetSocketOption(IPPROTO_TCP, TCP_NODELAY, (char*)&flag, sizeof(int));
        m_worldSocket->SetBlockingEnabled(false);
    }

    int WorldManager::ConnectToWorldServer(const std::string& ip, uint16_t port)
    {
        if (m_isConnecting)
            return 0;

        struct in_addr addr;
        if (inet_pton(AF_INET, ip.c_str(), &addr) != 1)
        {
            LOG_ERROR("Invalid world server IP: {}", ip);
            engine.GetConsole().Log("Invalid IP address.");
            return -1;
        }

        SocketAddress worldAddr(AF_INET, addr.s_addr, port);
        m_worldSocket->SetRemoteAddressAndPort(worldAddr, port);

        m_worldSocketConnected = false;
        m_isConnecting = true;

        if (m_worldSocket->Connect(worldAddr) != 0)
        {
            Console& c = engine.GetConsole();

            LOG_ERROR("Error while attempting to connect to world server.");
            c.Log("World connection failed! Server is down or not accepting connections.");
            OnDisconnect();
            return -1;
        }

        LOG_DEBUG("Attempting to connect to World Server...");

        // Initialize the world socket pollfd
        m_worldSocket->m_pfd.fd = m_worldSocket->GetSocketFD();
        m_worldSocket->m_pfd.events = POLLOUT;
        m_worldSocket->m_pfd.revents = 0;

        return 0;
    }

    int WorldManager::NetworkUpdate()
    {
        // If operations never start, no need to poll
        if (!m_isConnecting && !m_worldSocketConnected)
            return 0;

        std::vector<pollfd> m_pollList;
        m_pollList.push_back(m_worldSocket->m_pfd);

        static int timeout = 0; // waits 0 ms for the poll: try get data now or try next time

        int res = WSAPoll(m_pollList.data(), m_pollList.size(), timeout);

        // Check for errors
        if (res < 0)
        {
            LOG_ERROR("Could not Poll() WorldSocket.");
            OnDisconnect();
            return -1;
        }

        // Check for timeout
        if (res == 0)
            return 0;

        // Wait for connection
        if (!m_worldSocketConnected)
        {
            int res = CheckIfWorldConnected(m_pollList[0]); // m_pollList[0] is the authSocket
            if (res == -1)
            {
                LOG_DEBUG("Failed to connect to WorldServer.");
                OnDisconnect();
                return res;
            }
        }
        else // Connection has been estabilished
        {
            int r = CheckForIncomingData(m_pollList[0]);
            if (r == -1)
            {
                OnDisconnect();
                return r;
            }
        }

        return 0;
    }

    void WorldManager::OnDisconnect()
    {
        // Delete and recreate socket for next try
        m_worldSocket->Close();
        m_worldSocket.reset();

        m_data.Zero();

        CreateWorldSocket();

        m_worldSocketConnected = false;
        m_isConnecting = false;
    }

    int WorldManager::CheckIfWorldConnected(pollfd& pfd)
    {
        Console& c = engine.GetConsole();

        // Check for errors
        if (pfd.revents & (POLLERR | POLLHUP | POLLNVAL))
        {
            int error = 0;
            int len = sizeof(error);
            if (getsockopt(pfd.fd, SOL_SOCKET, SO_ERROR, (char*)&error, &len) == 0)
            {
                // Get error type and print a message
                if (error == WSAECONNREFUSED)
                {
                    LOG_ERROR("World connection refused: server is down or not accepting connections.");
                    c.Log("World connection refused: server is down or not accepting connections.");
                }
                else
                {
                    LOG_ERROR("WorldSocket encountered an error! {}", error);
                    c.Log("World connection lost! Server may have crashed or kicked you.");
                }
            }
            else
            {
                LOG_ERROR("getsockopt failed on world socket! [{}]", SocketUtility::GetLastError());
                c.Log("World connection lost! Server may have crashed or kicked you.");
            }

            return -1;
        }

        // Check if the socket is ready for writing, means we either connected successfuly or failed to connect
        if (pfd.revents & POLLOUT)
        {
            // If we get here, it indicates that a non-blocking connect has completed
            int error = 0;
            int len = sizeof(error);
            if (getsockopt(pfd.fd, SOL_SOCKET, SO_ERROR, (char*)&error, &len) < 0)
            {
                LOG_ERROR("getsockopt failed on world socket! [{}]", SocketUtility::GetLastError());
                return -1;
            }
            else if (error != 0)
            {
                LOG_ERROR("Socket error after world connect! [{}]", error);

                // Handle connection error
                if (error == WSAECONNREFUSED)
                    LOG_ERROR("World server refused the connection!");

                engine.GetConsole().Log("Server refused the connection!");

                return -1;
            }
            else
            {
                LOG_OK("Connected to the world server!");
                c.Log("Connected to the world server!");

                // Switch from POLLOUT to POLLIN to get incoming data, POLLOUT will also be checked but only if there are packets to send in the outQueue
                pfd.events = POLLIN;

                OnConnectedToWorldServer();

                return 0;
            }
        }

        return 0;
    }

    int WorldManager::OnConnectedToWorldServer()
    {
        m_isConnecting = false;
        m_worldSocketConnected = true;

        // No TLS setup, start handling the communication
        m_worldSocket->OnConnectedCallback(); // sends the [GREETCODE | ENCRYPTED_PACKET] greet

        return 0;
    }

    int WorldManager::CheckForIncomingData(pollfd& pfd)
    {
        Console& c = engine.GetConsole();

        if (pfd.revents & (POLLERR | POLLHUP | POLLNVAL))
        {
            int error = 0;
            int len = sizeof(error);
            if (getsockopt(pfd.fd, SOL_SOCKET, SO_ERROR, (char*)&error, &len) == 0)
            {
                LOG_ERROR("WorldSocket encountered an error! {}", error);
                c.Log("World connection lost! Server may have crashed or kicked you.");
            }
            else
            {
                LOG_ERROR("getsockopt failed on world socket! [{}]", SocketUtility::GetLastError());
                c.Log("World connection lost! Server may have crashed or kicked you.");
            }

            return -1;
        }
        else
        {
            if (pfd.revents & POLLOUT)
            {
                int r = m_worldSocket->Send();

                if (r < 0)
                {
                    int errCode = WSAGetLastError();
                    LOG_ERROR("Send: World socket error/disconnection detected. {}", errCode);
                    return -1;
                }
            }

            if (pfd.revents & POLLIN)
            {
                int r = m_worldSocket->Receive();

                if (r < 0)
                {
                    int errCode = WSAGetLastError();
                    LOG_ERROR("Receive: World socket error/disconnection detected. {}", errCode);
                    return -1;
                }
            }
        }

        return 0;
    }

    // -------------------------------------------------------------------------------------------------
    // Runs after the world has been updated, this is where our own state is replicated to the server
    // -------------------------------------------------------------------------------------------------
    void WorldManager::GameUpdate(double deltaTime)
    {
        // Update the timers
        m_timerUpdatePosPacketSend += 1 * deltaTime;

        // Update position/direction
        if (m_data.isInWorld && !m_data.isLeavingWorld)
        {
            if (m_timerUpdatePosPacketSend >= PLAYER_MOVEMENT_UPDATE_INTERVAL_SECONDS)
            {
                SendPlayerMovementUpdate();
                m_timerUpdatePosPacketSend = 0.0f;
            }
        }
    }

}
}
