#include <iostream>

#include "NECROHammer.h"

int main()
{
	if (NECRO::Hammer::g_client.Init() == 0)
	{
		NECRO::Hammer::g_client.Start();
	}

	return 0;
}
