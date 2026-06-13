#pragma once

#include "Singleton.h"

#include <string>

namespace Bunny
{
class Config : public Singleton<Config>
{
  public:
    void loadConfigFile(const std::string& path);

    std::string mWindowName = "Bunny Engine";
    bool mIsFullScreen = false;
    int mWindowWidth = 1280;
    int mWindowHeight = 720;
    std::string mModelFilePath = "./assets/model/both_smooth.glb";
    int mMultiSampleCount = 1;
};

} // namespace Bunny
