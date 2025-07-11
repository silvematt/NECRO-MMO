#include "AI.h"
#include "AIBehavior.h"

namespace NECRO
{
namespace Client
{
	//--------------------------------------------------
	// Initializes the AI, setting states and Behaviors
	//--------------------------------------------------
	int AI::Init()
	{
		state = AIStates::IDLE;

		// Construct the States->Behavior link
		// Idle = 0
		behaviorsPtrs.push_back(&AIBehavior::BehaviorIdle);

		// Others...

		return 0;
	}

	//--------------------------------------------------
	// AI Update cycle
	//--------------------------------------------------
	void AI::Update()
	{
		// AI Base update
		tilesetYOff = static_cast<int>(isoDirection);

		// TODO: we may not need do behaviors update not every frame
		// Run the behavior in base of the current state
		if (behaviorsPtrs.size() > static_cast<int>(state))
			behaviorsPtrs[static_cast<int>(state)](this);

		// Update the entity base, pos, gridPos, isoPos etc.
		Entity::Update();
	}

}
}
