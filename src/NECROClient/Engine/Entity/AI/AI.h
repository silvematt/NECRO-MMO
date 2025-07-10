#ifndef NECROAI_H
#define NECROAI_H

#include "Game.h"
#include "Entity.h"
#include <string>

//-------------------------------------------------
// AI class, derived by Entity
//-------------------------------------------------
class AI : public Entity
{
public:
	enum AIStates
	{
		IDLE = 0,
		ROAMING,
		PATROL,
		CHASE,
		ATTACK,
		LAST_VALUE
	};

public:
	AI() = default;

private:
	std::string name;

	AIStates state = IDLE;
	std::vector<void (*)(AI* owner)> behaviorsPtrs;	// Behavior routines of the AI for each state

	float baseSpeed = 2.5f;
	IsoDirection isoDirection = IsoDirection::SOUTH;

public:
	int			Init();
	void		Update() override;
};

#endif
