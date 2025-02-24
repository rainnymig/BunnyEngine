#include "Config.h"

#include "Window.h"

#include <fmt/core.h>
#include <inicpp.h>
#include <entt/entt.hpp>

using namespace Bunny::Engine;

int main(void)
{
    fmt::print("Welcome to Bunny Engine!\n");

#ifdef _DEBUG
    fmt::print("This is DEBUG build.\n");
#else
    fmt::print("This is RELEASE build.\n");
#endif

    Config config;
    // config.loadConfigFile("./config.ini");

    Bunny::Base::Window window;
    window.initialize(config.mWindowWidth, config.mWindowHeight, config.mIsFullScreen, config.mWindowName);

    bool shouldRun = true;

    while(shouldRun)
    {
        shouldRun = !window.processWindowEvent();
    }

    window.destroyAndTerminate();

    return 0;
}