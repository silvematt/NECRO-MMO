// NECROAuth

#include "NECROAuth.h"

int main()
{
	if (server.Init() == 0)
	{
		server.Start();

		server.Update();
	}

	return 0;
}
