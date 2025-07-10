// Include Standard Libraries
#include <iostream>

// Include NECRO
#include "NECROClient.h"

// Undefine SDL_main
#undef main

int main()
{
    SDL_Log("Booting up NECROClient...");

    if (engine.Init() == 0)
    {
        engine.Start();

        engine.Update();
    }

    SDL_Log("Exting application.");

    return 0;
}
