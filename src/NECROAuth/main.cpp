// NECROAuth Server

#include "NECROServer.h"

int main()
{
	if (NECRO::Auth::server.Init() == 0)
	{
		NECRO::Auth::server.Start();
		NECRO::Auth::server.Update();
	}

	return 0;
}
