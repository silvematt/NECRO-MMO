#ifndef NECRO_AUTH_MANAGER
#define NECRO_AUTH_MANAGER

#include "AuthSession.h"
#include "SocketUtility.h"
#include <array>

#include "AES.h"

#include <vector>

namespace NECRO
{
namespace Client
{
	struct RealmEntry
	{
		uint8_t id;
		uint8_t status;
		std::string ip;
		uint16_t port;
		std::string name;
	};

	struct AuthData
	{
		std::string username;
		std::string password;

		std::string ipAddress;

		std::array<uint8_t, AES_128_KEY_SIZE> sessionKey;
		std::array<uint8_t, AES_128_KEY_SIZE> greetcode;

		AES::IV iv;

		bool hasAuthenticated = false;

		std::vector<RealmEntry> realmlist;
	};

	class AuthManager
	{
	public:
		struct AuthConfigSettings
		{
			std::string hostname;
			uint16_t port;
		};

	private:
		AuthConfigSettings m_configSettings;
		AuthData m_data;

		std::unique_ptr<AuthSession> m_authSocket;
		bool m_authSocketConnected = false;
		bool m_isConnecting = false;

	public:
		int Init();
		void ApplySettings();

		void CreateAuthSocket();

		int ConnectToAuthServer();

		int CheckIfAuthConnected(pollfd& pfd);  // pfd of the authSession is passed as parameter
		int CheckForIncomingData(pollfd& pfd);
		int NetworkUpdate();

		void OnDisconnect();

		int OnConnectedToAuthServer();

		void OnAuthenticationCompleted();

		// AuthData Setters
		void SetAuthDataUsername(const std::string& u)
		{
			m_data.username = u;
		}

		void SetAuthDataPassword(const std::string& u)
		{
			m_data.password = u;
		}

		void SetAuthDataIpAddress(const std::string& i)
		{
			m_data.ipAddress = i;
		}

		AuthData& GetData()
		{
			return m_data;
		}
	};

}
}

#endif
