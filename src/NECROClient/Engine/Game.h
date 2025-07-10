#ifndef NECROGAME_H
#define NECROGAME_H

#include "World.h"
#include "Camera.h"
#include <string>

class Player;

//-------------------------------------------------------------------------------------
// For now GameMode refers to the modes in which we can place objects (edit) or 
// interact with things (play) 
//-------------------------------------------------------------------------------------
enum GameMode
{
	EDIT_MODE = 0,
	PLAY_MODE
};
extern std::string GameModeMap[];

enum IsoDirection
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
class NECROGame
{
private:
	GameMode curMode = GameMode::EDIT_MODE;

	World currentWorld;
	Camera mainCamera;
	Player* curPlayer;

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

inline void NECROGame::SetCurMode(GameMode m)
{
	curMode = m;
}

inline GameMode NECROGame::GetCurMode() const
{
	return curMode;
}

inline void NECROGame::SetCurPlayer(Player* p)
{
	curPlayer = p;
}

inline Camera* NECROGame::GetMainCamera()
{
	return &mainCamera;
}

inline World* NECROGame::GetCurrentWorld()
{
	return &currentWorld;
}

inline Player* NECROGame::GetCurPlayer()
{
	return curPlayer;
}

#endif
