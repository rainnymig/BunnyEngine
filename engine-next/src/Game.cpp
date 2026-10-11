#include "Game.h"

#include "World.h"

namespace Bunny
{
BunnyResult Game::initialize(Engine& engine)
{
    mEngine = &engine;

    mWorld = std::make_unique<World>();

    return BUNNY_HAPPY;
}

BunnyResult Game::run()
{
    return BunnyResult();
}

BunnyResult Game::shutdown()
{
    return BunnyResult();
}
} // namespace Bunny