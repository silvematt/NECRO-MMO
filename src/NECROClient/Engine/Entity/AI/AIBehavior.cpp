#include "AIBehavior.h"
#include "NECROEngine.h"

namespace NECRO
{
namespace Client
{
	void AIBehavior::BehaviorIdle(AI* owner)
	{
		// TEST: Just for testing, move the entity
		owner->m_pos.x += 1 * engine.GetDeltaTime() * 100;
		owner->m_pos.y += 1 * engine.GetDeltaTime() * 100;
	}

}
}
