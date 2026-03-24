// NECROWorld Server

#include "NECROWorld.h"

int main()
{
	auto& server = NECRO::World::Server::Instance();

	if (server.Init() == 0)
	{
		server.Start();
		server.Update();
	}

	return 0;
}
