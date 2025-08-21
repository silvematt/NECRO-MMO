#include <iostream>

#include "NECROHammer.h"

int main()
{
	auto& client = NECRO::Hammer::Client::Instance();

	if (client.Init() == 0)
	{
		client.Start();
		client.Update();
	}

	return 0;
}
