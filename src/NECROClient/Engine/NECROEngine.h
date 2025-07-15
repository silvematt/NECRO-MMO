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
	inline constexpr uint8_t CLIENT_VERSION_MAJOR = 1;
	inline constexpr uint8_t CLIENT_VERSION_MINOR = 0;
	inline constexpr uint8_t CLIENT_VERSION_REVISION = 0;

	class Engine
	{
	private:
		// Status
		bool		m_isRunning;
		double		m_deltaTime;
		uint32_t	m_lastUpdate;
		float		m_fps;

		// Game
		Game m_game;

		// Subsystems
		Input			m_input;
		Renderer		m_renderer;
		AssetsManager	m_assetsManager;
		Console			m_console;
		AuthManager		m_netManager;

		int				Shutdown();

	public:
		Game&				GetGame();
		Input&				GetInput();
		Renderer&			GetRenderer();
		AssetsManager&		GetAssetsManager();
		Console&			GetConsole();
		AuthManager&		GetAuthManager();

		const double		GetDeltaTime() const;
		const float			GetFPS() const;

		int					Init();
		void				Start();
		void				Update();
		void				Stop();
	};


	// Global access for the Engine 
	extern Engine engine;

	// Inline functions
	inline Game& Engine::GetGame()
	{
		return m_game;
	}

	inline Input& Engine::GetInput()
	{
		return m_input;
	}

	inline Renderer& Engine::GetRenderer()
	{
		return m_renderer;
	}

	inline AssetsManager& Engine::GetAssetsManager()
	{
		return m_assetsManager;
	}

	inline Console& Engine::GetConsole()
	{
		return m_console;
	}

	inline AuthManager& Engine::GetAuthManager()
	{
		return m_netManager;
	}

	inline const double Engine::GetDeltaTime() const
	{
		return m_deltaTime;
	}

	inline const float Engine::GetFPS() const
	{
		return m_fps;
	}

}
}

#endif
