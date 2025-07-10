#ifndef NECROINTERACTABLE_H
#define NECROINTERACTABLE_H

#include <string>

class Entity;

//---------------------------------------------------------------------------------------------
// This class is attached to Entities that can be interacted with, upon interaction, a specific
// function is triggered in base of the selected InteractType.
//---------------------------------------------------------------------------------------------
class Interactable
{
public:
	// All types of actions that can be performed when an Entity is being interacted with
	enum InteractType
	{
		DESTROY = 0,
		TOGGLE_LIGHT,
		TOGGLE_COLL,
		PLAY_ANIM,
		MOVE,
		MOVE_RELATIVE,
		LAST_VAL
	};

public:
	Interactable(Entity* e, InteractType t = InteractType::DESTROY)
	{
		parStr = "";
		parFloat1 = 0.0f;
		parFloat2 = 0.0f;

		owner = e;
		type = t;
	}

private:
	Entity* owner;

	// All types of interacitons, prefixed IN for INteraction
	void INDestroy();
	void INToggleLight();
	void INToggleColl();
	void INPlayAnim(std::string state);
	void INMove(float x, float y);
	void INMoveRelative(float x, float y);

public:
	InteractType type;

	int gridDistanceInteraction = 1; // the distance (in grid) between the interactor (player) and this interactable that needs to be respected to trigger the Interact call
									 // if 0, it means any distance

	// Parameters
	std::string parStr;
	float parFloat1;
	float parFloat2;

	// Called upon interaction
	void Interact();
};

#endif
