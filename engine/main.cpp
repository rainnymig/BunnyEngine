#include "Engine.h"

#include <iostream>

using namespace Bunny;

int main(void)
{
    std::cout << "Welcome to Bunny Engine!\n";

#ifdef _DEBUG
    std::cout << "It's debug mode\n";
#else
    std::cout << "It's release mode\n";
#endif

    Engine engine;

    engine.run();

    return 0;
}