#include <iostream>

#include "NECROClient.h"
#include "World.h"

NECROClient engine;

//--------------------------------------
// Initializes the engine and subsystems
//--------------------------------------
int NECROClient::Init()
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
	if (input.Init() != 0)
	{
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to Initialize Input SubSystem\n");
		return -4;
	}

	// Initialize Renderer Subsystem
	if (renderer.Init() != 0)
	{
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to Initialize Renderer SubSystem\n");
		return -5;
	}

	// Initialize AssetsManager SubSystem
	if (assetsManager.Init() != 0)
	{
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to Initialize AssetsManager SubSystem\n");
		return -6;
	}

	// Inizialize Console
	if (console.Init() != 0)
	{
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to Initialize Console SubSystem.\n");
		return -7;
	}

	// Initialize Network
	if (netManager.Init() != 0)
	{
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to Initialize Network SubSystem.\n");
		return -8;
	}

	srand(time(NULL));

	lastUpdate = 0;
	deltaTime = 0;
	fps = 0;

	SDL_Log("Initializing done.\n");
	return 0;
}

//--------------------------------------
// Shuts down the engine and subsystems
//--------------------------------------
int NECROClient::Shutdown()
{
	SDL_Log("Shutting down the engine...");

	// Shutdown subsystem
	renderer.Shutdown();
	console.Shutdown();

	// Shutdown SDL
	SDL_Quit();
	IMG_Quit();
	TTF_Quit();

	return 0;
}

//--------------------------------------
// Stops the engine at the next Update
//--------------------------------------
void NECROClient::Stop()
{
	SDL_Log("Stopping the engine...");

	isRunning = false;
}

//--------------------------------------------------------------------------
// Start happens after the engine is initialized and before the engine loop
//--------------------------------------------------------------------------
void NECROClient::Start()
{
	SDL_Log("Engine is running.");

	isRunning = true;

	// Initialize game
	game.Init();

	lastUpdate = SDL_GetTicks();
}


//--------------------------------------
// Engine Update
//--------------------------------------
void NECROClient::Update()
{
	// Engine Loop
	while (isRunning)
	{
		Uint64 start = SDL_GetPerformanceCounter();
		uint32_t current = SDL_GetTicks();
		deltaTime = (current - lastUpdate) / 1000.0f;
		lastUpdate = current;

		input.Handle();
		renderer.Clear();

		game.Update();

		renderer.Update();
		renderer.Show();

		Uint64 end = SDL_GetPerformanceCounter();
		float elapsed = (end - start) / (float)SDL_GetPerformanceFrequency();
		fps = (1.0f / elapsed);
	}

	// Shutdown
	game.Shutdown();
	Shutdown();
}
