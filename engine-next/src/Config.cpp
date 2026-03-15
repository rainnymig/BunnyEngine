#include "Config.h"

#include <inicpp.h>

namespace Bunny::Base
{
//  to avoid Error C2888
//  A symbol belonging to namespace A must be defined in a namespace that encloses A.
std::unique_ptr<Bunny::Engine::Config> Bunny::Engine::Config::msInstance = nullptr;
} // namespace Bunny::Base

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
    mModelFilePath = basicSection["modelFilePath"].as<std::string>();
    mMultiSampleCount = basicSection["multiSampleCount"].as<int>();
}

} // namespace Bunny::Engine
