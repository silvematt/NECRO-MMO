// NECROServer

#include "NECROServer.h"

int main()
{
	if (server.Init() == 0)
	{
		server.Update();
	}

	return 0;
}
