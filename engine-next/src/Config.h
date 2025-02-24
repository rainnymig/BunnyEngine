#pragma once

#include <string>

namespace Bunny::Engine
{
class Config
{
  public:
    void loadConfigFile(const std::string& path);

    std::string mWindowName = "Bunny Engine";
    bool mIsFullScreen = false;
    int mWindowWidth = 1280;
    int mWindowHeight = 720;
};

} // namespace Bunny::Engine
