#ifndef NECRO_AUTH_MANAGER
#define NECRO_AUTH_MANAGER

#include "AuthSession.h"
#include "SocketUtility.h"
#include <array>

#include "AES.h"

struct AuthData
{
	std::string username;
	std::string password;

	std::string ipAddress;

	std::array<uint8_t, AES_128_KEY_SIZE> sessionKey;
	std::array<uint8_t, AES_128_KEY_SIZE> greetcode;

	NECROAES::IV iv;

	bool hasAuthenticated = false;
};

class NECROAuthManager
{
private:
	AuthData data;

	std::unique_ptr<AuthSession> authSocket;
	bool authSocketConnected = false;
	bool isConnecting = false;

	std::vector<pollfd> poll_fds;


public:
	int Init();
	void CreateAuthSocket();

	int ConnectToAuthServer();
	
	int CheckIfAuthConnected();
	int CheckForIncomingData();
	int NetworkUpdate();

	void OnDisconnect();

	int OnConnectedToAuthServer();

	void OnAuthenticationCompleted();

	// AuthData Setters
	void SetAuthDataUsername(const std::string& u)
	{
		data.username = u;
	}

	void SetAuthDataPassword(const std::string& u)
	{
		data.password = u;
	}

	void SetAuthDataIpAddress(const std::string& i)
	{
		data.ipAddress = i;
	}

	AuthData& GetData()
	{
		return data;
	}
};


#endif
