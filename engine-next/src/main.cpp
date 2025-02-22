#include "Config.h"

#include <fmt/core.h>
#include <inicpp.h>

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
    config.loadConfigFile("./config.ini");

    fmt::print("width is {}, fullscreen is {}\n", config.mWindowWidth, config.mIsFullScreen);

    return 0;
}