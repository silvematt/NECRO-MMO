#ifndef NECROINTERACTABLE_H
#define NECROINTERACTABLE_H

#include <string>

namespace NECRO
{
namespace Client
{
	class Entity;

	//---------------------------------------------------------------------------------------------
	// This class is attached to Entities that can be interacted with, upon interaction, a specific
	// function is triggered in base of the selected InteractType.
	//---------------------------------------------------------------------------------------------
	class Interactable
	{
	public:
		// All types of actions that can be performed when an Entity is being interacted with
		enum class InteractType
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
			m_parStr = "";
			m_parFloat1 = 0.0f;
			m_parFloat2 = 0.0f;

			m_owner = e;
			m_type = t;
		}

	private:
		Entity* m_owner;

		// All types of interacitons, prefixed IN for INteraction
		void INDestroy();
		void INToggleLight();
		void INToggleColl();
		void INPlayAnim(std::string state);
		void INMove(float x, float y);
		void INMoveRelative(float x, float y);

	public:
		InteractType m_type;

		int m_gridDistanceInteraction = 1; // the distance (in grid) between the interactor (player) and this interactable that needs to be respected to trigger the Interact call
		// if 0, it means any distance

		// Parameters
		std::string m_parStr;
		float m_parFloat1;
		float m_parFloat2;

		// Called upon interaction
		void Interact();
	};

}
}

#endif
