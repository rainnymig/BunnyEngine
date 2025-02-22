#include "Config.h"

#include <inicpp.h>

#include <string>

namespace Bunny::Engine
{
void Config::loadConfigFile(std::string_view path)
{
    std::string pathStr(path);
    ini::IniFile loadedIni(pathStr);

    auto& basicSection = loadedIni["basic"];
    mIsFullScreen = basicSection["isFullscreen"].as<bool>();
    mWindowHeight = basicSection["windowHeight"].as<int>();
    mWindowWidth = basicSection["windowWidth"].as<int>();
}

} // namespace Bunny::Engine
