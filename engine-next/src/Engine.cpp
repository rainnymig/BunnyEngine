#include "Engine.h"

#include "Config.h"
#include "GraphicsSystem.h"
#include "Game.h"
#include "ImguiHelper.h"

namespace Bunny
{

BunnyResult Engine::initialize(std::unique_ptr<Game>&& game)
{
    mGame = std::move(game);

    mWindow = std::make_unique<Window>();
    mWindow->initialize(Config::get().mWindowWidth, Config::get().mWindowHeight, Config::get().mIsFullScreen,
        Config::get().mWindowName);

    mInputManager = std::make_unique<InputManager>();
    mInputManager->setupWithWindow(*mWindow);

    ImguiHelper::setup();

    return BUNNY_HAPPY;
}

BunnyResult Engine::runEngine()
{

    return BUNNY_HAPPY;
}

BunnyResult Engine::shutdown()
{
    return BUNNY_HAPPY;
}

} // namespace Bunny
