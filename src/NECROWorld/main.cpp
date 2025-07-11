// NECROWorld Server

#include "NECROWorld.h"

int main()
{
	if (NECRO::World::server.Init() == 0)
	{
		NECRO::World::server.Update();
	}

	return 0;
}
