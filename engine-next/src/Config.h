#pragma once

#include <string_view>

namespace Bunny::Engine
{
class Config
{
  public:
    void loadConfigFile(std::string_view path);

    bool mIsFullScreen = false;
    int mWindowWidth = 1280;
    int mWindowHeight = 720;
};

} // namespace Bunny::Engine
