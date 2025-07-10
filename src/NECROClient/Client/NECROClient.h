#ifndef NECROCLIENT_H
#define NECROCLIENT_H

#include "SDL.h"

#include "Game.h"
#include "Input.h"
#include "AssetsManager.h"
#include "Renderer.h"
#include "Console.h"
#include "NMath.h"
#include "AuthManager.h"

constexpr uint8_t CLIENT_VERSION_MAJOR = 1;
constexpr uint8_t CLIENT_VERSION_MINOR = 0;
constexpr uint8_t CLIENT_VERSION_REVISION = 0;

class NECROClient
{
private:
	// Status
	bool isRunning;
	double deltaTime;
	uint32_t lastUpdate;
	float fps;

	// Game
	NECROGame game;

	// Subsystems
	NECROInput			input;
	NECRORenderer		renderer;
	NECROAssetsManager	assetsManager;
	NECROConsole		console;
	NECROAuthManager		netManager;

	int						Shutdown();
	
public:
	NECROGame&				GetGame();

	NECROInput&				GetInput();
	NECRORenderer&			GetRenderer();
	NECROAssetsManager&		GetAssetsManager();
	NECROConsole&			GetConsole();
	NECROAuthManager&		GetAuthManager();

	const double			GetDeltaTime() const;
	const float				GetFPS() const;

	int						Init();
	void					Start();
	void					Update();
	void					Stop();
};


// Global access for the Engine 
extern NECROClient engine;

// Inline functions
inline NECROGame& NECROClient::GetGame()
{
	return game;
}

inline NECROInput& NECROClient::GetInput()
{
	return input;
}

inline NECRORenderer & NECROClient::GetRenderer() 
{
	return renderer;
}

inline NECROAssetsManager& NECROClient::GetAssetsManager()
{
	return assetsManager;
}

inline NECROConsole& NECROClient::GetConsole()
{
	return console;
}

inline NECROAuthManager& NECROClient::GetAuthManager()
{
	return netManager;
}

inline const double NECROClient::GetDeltaTime() const
{
	return deltaTime;
}

inline const float NECROClient::GetFPS() const
{
	return fps;
}

#endif
