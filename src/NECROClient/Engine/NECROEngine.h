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

namespace NECRO
{
namespace Client
{
	constexpr uint8_t CLIENT_VERSION_MAJOR = 1;
	constexpr uint8_t CLIENT_VERSION_MINOR = 0;
	constexpr uint8_t CLIENT_VERSION_REVISION = 0;

	class Engine
	{
	private:
		// Status
		bool isRunning;
		double deltaTime;
		uint32_t lastUpdate;
		float fps;

		// Game
		Game game;

		// Subsystems
		Input			input;
		Renderer		renderer;
		AssetsManager	assetsManager;
		Console		console;
		AuthManager		netManager;

		int						Shutdown();

	public:
		Game& GetGame();

		Input& GetInput();
		Renderer& GetRenderer();
		AssetsManager& GetAssetsManager();
		Console& GetConsole();
		AuthManager& GetAuthManager();

		const double			GetDeltaTime() const;
		const float				GetFPS() const;

		int						Init();
		void					Start();
		void					Update();
		void					Stop();
	};


	// Global access for the Engine 
	extern Engine engine;

	// Inline functions
	inline Game& Engine::GetGame()
	{
		return game;
	}

	inline Input& Engine::GetInput()
	{
		return input;
	}

	inline Renderer& Engine::GetRenderer()
	{
		return renderer;
	}

	inline AssetsManager& Engine::GetAssetsManager()
	{
		return assetsManager;
	}

	inline Console& Engine::GetConsole()
	{
		return console;
	}

	inline AuthManager& Engine::GetAuthManager()
	{
		return netManager;
	}

	inline const double Engine::GetDeltaTime() const
	{
		return deltaTime;
	}

	inline const float Engine::GetFPS() const
	{
		return fps;
	}

}
}

#endif
