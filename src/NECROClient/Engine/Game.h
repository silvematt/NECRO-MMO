#ifndef NECROGAME_H
#define NECROGAME_H

#include "World.h"
#include "Camera.h"
#include <string>

namespace NECRO
{
namespace Client
{
	class Player;

	//-------------------------------------------------------------------------------------
	// For now GameMode refers to the modes in which we can place objects (edit) or 
	// interact with things (play) 
	//-------------------------------------------------------------------------------------
	enum class GameMode
	{
		EDIT_MODE = 0,
		PLAY_MODE
	};
	extern std::string g_GameModeMap[];

	enum class IsoDirection
	{
		WEST = 0,
		NORTH_WEST,
		NORTH,
		NORTH_EAST,
		EAST,
		SOUTH_EAST,
		SOUTH,
		SOUTH_WEST
	};

	//-------------------------------------------------------
	// Defines the Game related things that run in the engine
	//-------------------------------------------------------
	class Game
	{
	private:
		GameMode	m_curMode = GameMode::EDIT_MODE;

		World		m_currentWorld;
		Camera		m_mainCamera;
		Player*		m_curPlayer;

	private:
		void		HandleInput();

	public:
		void		Init();
		void		Update();
		void		Shutdown();

		void		SetCurMode(GameMode m);
		void		SetCurPlayer(Player* p);

		GameMode	GetCurMode() const;
		Camera*		GetMainCamera();
		World*		GetCurrentWorld();
		Player*		GetCurPlayer();
	};

	inline void Game::SetCurMode(GameMode m)
	{
		m_curMode = m;
	}

	inline GameMode Game::GetCurMode() const
	{
		return m_curMode;
	}

	inline void Game::SetCurPlayer(Player* p)
	{
		m_curPlayer = p;
	}

	inline Camera* Game::GetMainCamera()
	{
		return &m_mainCamera;
	}

	inline World* Game::GetCurrentWorld()
	{
		return &m_currentWorld;
	}

	inline Player* Game::GetCurPlayer()
	{
		return m_curPlayer;
	}

}
}

#endif
