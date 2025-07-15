#include "Game.h"
#include "NECROEngine.h"

namespace NECRO
{
namespace Client
{
	std::string g_GameModeMap[] =
	{
		"EDIT_MODE",
		"PLAY_MODE"
	};

	//--------------------------------------
	// Initialize the game
	//--------------------------------------
	void Game::Init()
	{
		m_currentWorld.InitializeWorld();
	}

	//--------------------------------------
	// Game Update
	//--------------------------------------
	void Game::Update()
	{
		engine.GetAuthManager().NetworkUpdate();

		HandleInput();

		m_mainCamera.Update();
		m_currentWorld.Update();
		m_currentWorld.Draw();

		m_mainCamera.RenderVisibleEntities();

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
		if (input.GetKeyDown(SDL_SCANCODE_TAB))
		{
			switch (m_curMode)
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
