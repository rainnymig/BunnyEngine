#include "VulkanRenderer.h"
#include "Engine.h"

#include <iostream>

using namespace Bunny;

int main(void)
{
    std::cout << "Welcome to Bunny Engine!\n";

    Engine engine;

    engine.run();

    return 0;
}