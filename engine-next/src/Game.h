#pragma once

#include "BunnyResult.h"

#include <memory>

namespace Bunny
{

class Engine;
class World;

class Game
{
  public:
    virtual BunnyResult initialize(Engine& engine);
    virtual BunnyResult run();
    virtual BunnyResult shutdown();

    World& getWorld() { return *mWorld; }

  private:
    std::unique_ptr<World> mWorld;

    Engine* mEngine;
};
} // namespace Bunny