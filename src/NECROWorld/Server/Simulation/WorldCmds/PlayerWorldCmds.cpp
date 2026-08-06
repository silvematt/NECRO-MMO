#include "WorldSimulation.h"
#include "WorldCmdTypes.h"

#include "CharacterData.h"

namespace NECRO
{
namespace World
{
	PlayerSpawnCmdResult WorldSimulation::WorldCmd_TryToSpawnPlayerCharacter(std::shared_ptr<CharacterData> charData)
	{
		return PlayerSpawnCmdResult();
	}
}
}
