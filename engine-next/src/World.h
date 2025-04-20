#pragma once

#include <entt/entt.hpp>

namespace Bunny::Engine
{

class World
{
  public:
    void update(float deltaTime);

    entt::registry mEntityRegistry;
};
} // namespace Bunny::Engine
