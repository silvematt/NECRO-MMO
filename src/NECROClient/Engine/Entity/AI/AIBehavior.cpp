#include "AIBehavior.h"

namespace NECRO
{
namespace Client
{
	void AIBehavior::BehaviorIdle(AI* owner)
	{
		// TEST: Just for testing, move the entity
		owner->m_pos.x += 1;
		owner->m_pos.y += 1;
	}

}
}
