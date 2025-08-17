#include <iostream>

#include "NECROHammer.h"

int main()
{
	if (NECRO::Hammer::Client::Instance().Init() == 0)
	{
		NECRO::Hammer::Client::Instance().Start();
		NECRO::Hammer::Client::Instance().Update();
	}

	return 0;
}
