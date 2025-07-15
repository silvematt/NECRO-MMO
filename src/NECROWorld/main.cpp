// NECROWorld Server

#include "NECROWorld.h"

int main()
{
	if (NECRO::World::g_server.Init() == 0)
	{
		NECRO::World::g_server.Update();
	}

	return 0;
}
