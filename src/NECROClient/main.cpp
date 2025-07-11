// Include Standard Libraries
#include <iostream>

// Include NECRO
#include "NECROEngine.h"

// Undefine SDL_main
#undef main

int main()
{
    SDL_Log("Booting up NECROClient...");

    if (NECRO::Client::engine.Init() == 0)
    {
        NECRO::Client::engine.Start();
        NECRO::Client::engine.Update();
    }

    SDL_Log("Exting application.");

    return 0;
}
