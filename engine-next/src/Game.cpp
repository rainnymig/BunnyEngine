#include "Game.h"

namespace Bunny
{
BunnyResult Game::initialize(Engine& engine)
{
    mEngine = &engine;

    return BUNNY_HAPPY;
}
} // namespace Bunny