#include "Engine.h"

#include <Vector3.h>

#include <iostream>

using namespace Bunny;

int main(void)
{
    std::cout << "Welcome to Bunny Engine!\n";

    ::Physics::Vector3 v1(2, 3, 4);
    ::Physics::Vector3 v2(2, 4, 8);

    Engine engine;

    engine.run();

    return 0;
}