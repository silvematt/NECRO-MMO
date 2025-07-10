#ifndef NECROAI_BEHAVIOR_H
#define NECROAI_BEHAVIOR_H


#include "AI.h"
#include <string>

//------------------------------------------------------------------------------------------------------
// Each AI state is linked with an AIBehavior, that can be implemented as a static function.
// 
// This allows to write small and shareable behaviors between AI entities, while allowing for a more
// data-driven design for AI definitions, where we can have a file-specified "AIPrefab" that maps each
// state->behavior link.
//------------------------------------------------------------------------------------------------------
class AIBehavior
{
public:
	static void BehaviorIdle(AI* owner);
};


#endif
