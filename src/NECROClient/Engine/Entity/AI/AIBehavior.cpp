#include "AIBehavior.h"

namespace NECRO
{
namespace Client
{
	void AIBehavior::BehaviorIdle(AI* owner)
	{
		// TEST: Just for testing, move the entity
		owner->pos.x += 1;
		owner->pos.y += 1;
	}

}
}
