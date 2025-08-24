// NECROAuth Server

#include "NECROServer.h"

int main()
{
	auto& server = NECRO::Auth::Server::Instance();

	if (server.Init() == 0)
	{
		server.Start();
		server.Update();
	}

	return 0;
}
