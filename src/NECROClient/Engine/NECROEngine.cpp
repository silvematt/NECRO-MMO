#include <iostream>

#include "NECROEngine.h"
#include "World.h"

namespace NECRO
{
namespace Client
{
	Engine engine;

	//--------------------------------------
	// Initializes the engine and subsystems
	//--------------------------------------
	int Engine::Init()
	{
		SDL_Log("Initializing engine...\n");

		// Initialize SDL
		if (SDL_Init(SDL_INIT_EVERYTHING) != 0)
		{
			SDL_LogError(SDL_LOG_CATEGORY_ERROR, "SDL_Init Error: %s\n", SDL_GetError());
			return -1;
		}

		int imgFlags = IMG_INIT_PNG;
		// Img init returns 0 on failure
		if (IMG_Init(imgFlags) == 0)
		{
			SDL_LogError(SDL_LOG_CATEGORY_ERROR, "SDL IMG_Init Error\n");
			return -2;
		}

		if (TTF_Init() != 0)
		{
			SDL_LogError(SDL_LOG_CATEGORY_ERROR, "SDL TTF_Init Error\n");
			return -3;
		}

		// Initialize Input SubSystem
		if (m_input.Init() != 0)
		{
			SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to Initialize Input SubSystem\n");
			return -4;
		}

		// Initialize Renderer Subsystem
		if (m_renderer.Init() != 0)
		{
			SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to Initialize Renderer SubSystem\n");
			return -5;
		}

		// Initialize AssetsManager SubSystem
		if (m_assetsManager.Init() != 0)
		{
			SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to Initialize AssetsManager SubSystem\n");
			return -6;
		}

		// Inizialize Console
		if (m_console.Init() != 0)
		{
			SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to Initialize Console SubSystem.\n");
			return -7;
		}

		// Initialize Network
		if (m_netManager.Init() != 0)
		{
			SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to Initialize Network SubSystem.\n");
			return -8;
		}

		srand(time(NULL));

		m_lastUpdate = 0;
		m_deltaTime = 0;
		m_fps = 0;

		SDL_Log("Initializing done.\n");
		return 0;
	}

	//--------------------------------------
	// Shuts down the engine and subsystems
	//--------------------------------------
	int Engine::Shutdown()
	{
		SDL_Log("Shutting down the engine...");

		// Shutdown subsystem
		m_renderer.Shutdown();
		m_console.Shutdown();

		// Shutdown SDL
		SDL_Quit();
		IMG_Quit();
		TTF_Quit();

		return 0;
	}

	//--------------------------------------
	// Stops the engine at the next Update
	//--------------------------------------
	void Engine::Stop()
	{
		SDL_Log("Stopping the engine...");

		m_isRunning = false;
	}

	//--------------------------------------------------------------------------
	// Start happens after the engine is initialized and before the engine loop
	//--------------------------------------------------------------------------
	void Engine::Start()
	{
		SDL_Log("Engine is running.");

		m_isRunning = true;

		// Initialize game
		m_game.Init();

		m_lastUpdate = SDL_GetTicks();
	}


	//--------------------------------------
	// Engine Update
	//--------------------------------------
	void Engine::Update()
	{
		// Engine Loop
		while (m_isRunning)
		{
			Uint64 start = SDL_GetPerformanceCounter();
			uint32_t current = SDL_GetTicks();
			m_deltaTime = (current - m_lastUpdate) / 1000.0f;
			m_lastUpdate = current;

			m_input.Handle();
			m_renderer.Clear();

			m_game.Update();

			m_renderer.Update();
			m_renderer.Show();

			Uint64 end = SDL_GetPerformanceCounter();
			float elapsed = (end - start) / (float)SDL_GetPerformanceFrequency();
			m_fps = (1.0f / elapsed);
		}

		// Shutdown
		m_game.Shutdown();
		Shutdown();
	}

}
}
