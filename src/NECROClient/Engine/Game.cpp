#include "Game.h"
#include "NECROEngine.h"

namespace NECRO
{
namespace Client
{
	std::string GameModeMap[] =
	{
		"EDIT_MODE",
		"PLAY_MODE"
	};

	//--------------------------------------
	// Initialize the game
	//--------------------------------------
	void Game::Init()
	{
		currentWorld.InitializeWorld();
	}

	//--------------------------------------
	// Game Update
	//--------------------------------------
	void Game::Update()
	{
		engine.GetAuthManager().NetworkUpdate();

		HandleInput();

		mainCamera.Update();
		currentWorld.Update();
		currentWorld.Draw();

		mainCamera.RenderVisibleEntities();

		engine.GetConsole().Update();
	}

	//--------------------------------------
	// Shuts down game related things
	//--------------------------------------
	void Game::Shutdown()
	{

	}

	//--------------------------------------
	// Handles input for global game cmds
	//--------------------------------------
	void Game::HandleInput()
	{
		// Game CMDs
		Input& input = engine.GetInput();

		// Switch game mode
		if (engine.GetInput().GetKeyDown(SDL_SCANCODE_TAB))
		{
			switch (curMode)
			{
			case GameMode::EDIT_MODE:
				SetCurMode(GameMode::PLAY_MODE);
				break;

			case GameMode::PLAY_MODE:
				SetCurMode(GameMode::EDIT_MODE);
				break;
			}
		}
	}

}
}
