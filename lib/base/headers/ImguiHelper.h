#pragma once

#include "Singleton.h"

#include <functional>
#include <vector>

namespace Bunny::Base
{
class ImguiHelper : public Singleton<ImguiHelper>
{
  public:
    using ImguiCommand = std::function<void()>;

    void registerCommand(ImguiCommand command);
    void render() const;

  private:
    std::vector<ImguiCommand> mRegisteredCommands;
};
} // namespace Bunny::Base
