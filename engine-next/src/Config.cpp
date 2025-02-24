#include "Config.h"

#include <inicpp.h>

namespace Bunny::Engine
{
void Config::loadConfigFile(const std::string& path)
{
    ini::IniFile loadedIni(path);

    auto& basicSection = loadedIni["basic"];
    mWindowName = basicSection["windowName"].as<std::string>();
    mIsFullScreen = basicSection["isFullscreen"].as<bool>();
    mWindowHeight = basicSection["windowHeight"].as<int>();
    mWindowWidth = basicSection["windowWidth"].as<int>();
}

} // namespace Bunny::Engine
