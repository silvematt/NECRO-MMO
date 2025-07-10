#include "NECROAuth.h"

#include "SocketUtility.h"
#include "OpenSSLManager.h"
#include "TCPSocketManager.h"
#include "Packet.h"

#include <memory>

#include <openssl/ssl.h>


// -------------------------------------------------------------------------------------------------------------------------------------------------
// NECROAuthentication
// 
// The way the authentication works is the following: to authenticate, users will connect to the NECROAuth Server, providing login, password, and 
// other info. The connection to the NECROAuth server will be done via TLS. When connection is granted, a sessionKey is generated and put
// inside the database. The client will securely receive the sessionKey as well (while still being in TLS), and then receive the realmlist that will
// allow the user to connect to the game server.
// 
// The game server will NOT use TLS, instead, packets will be encrypted/decrypted by each end using the sessionKey, hashed with some random
// bytes client and server will send to each other. Once they both have their shared secrets, world packets will be encrypted via AES with that 
// encryption key. In this way we can make sure the Server receives packets from the actual authorized user, and users are able to only get and read
// packets that are destinated to them.
// -------------------------------------------------------------------------------------------------------------------------------------------------

NECROAuth server;

int NECROAuth::Init()
{
	isRunning = false;

	LOG_OK("Booting up NECROAuth...");

	SocketUtility::Initialize();
	
	if (OpenSSLManager::ServerInit() != 0)
		return -1;

	if (directdb.Init() != 0)
	{
		LOG_ERROR("Could not initialize directdb, MySQL may be not running.");
		return -2;
	}

	if (dbworker.Setup(Database::DBType::LOGIN_DATABASE) != 0)
	{
		LOG_ERROR("Could not initialize directdb, MySQL may be not running.");
		return -3;
	}

	if (dbworker.Start() != 0)
	{
		LOG_ERROR("Could not start dbworker, MySQL may be not running.");
		return -4;
	}

	// Make TCPSocketManager
	sockManager = std::make_unique<TCPSocketManager>(SocketAddressesFamily::INET);

	return 0;
}

void NECROAuth::Start()
{
	isRunning = true;

	LOG_OK("NECROAuth is running...");
}

void NECROAuth::Update()
{
	// Server Loop
	while (isRunning)
	{
		int pollVal = sockManager->Poll();

		if (pollVal == -1)
			Stop();
	}

	Shutdown();
}

void NECROAuth::Stop()
{
	LOG_OK("Stopping NECROAuth...");

	isRunning = false;
}

int NECROAuth::Shutdown()
{
	// Shutdown
	LOG_OK("Shutting down NECROAuth...");

	directdb.Close();

	dbworker.Stop();
	dbworker.Join();
	dbworker.CloseDB();

	LOG_OK("Shut down of the NECROAuth completed.");
	return 0;
}
