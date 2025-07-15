// NECROAuth Server

#include "NECROServer.h"

int main()
{
	if (NECRO::Auth::g_server.Init() == 0)
	{
		NECRO::Auth::g_server.Start();
		NECRO::Auth::g_server.Update();
	}

	return 0;
}
