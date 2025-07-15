#ifndef NECROAI_H
#define NECROAI_H

#include "Game.h"
#include "Entity.h"
#include <string>

namespace NECRO
{
namespace Client
{
	//-------------------------------------------------
	// AI class, derived by Entity
	//-------------------------------------------------
	class AI : public Entity
	{
	public:
		enum class AIStates
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
		std::string m_name;

		AIStates							m_state = AIStates::IDLE;
		std::vector<void (*)(AI* owner)>	m_behaviorsPtrs;			// Behavior routines of the AI for each state

		float			m_baseSpeed = 2.5f;
		IsoDirection	m_isoDirection = IsoDirection::SOUTH;

	public:
		int			Init();
		void		Update() override;
	};

}
}

#endif
