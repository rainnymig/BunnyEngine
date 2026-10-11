#pragma once

#include "BunnyResult.h"

namespace Bunny
{

class Engine;

class Game
{
  public:
    virtual BunnyResult initialize(Engine& engine);
    virtual BunnyResult run();
    virtual BunnyResult shutdown();

  private:
    Engine* mEngine;
};
} // namespace Bunny