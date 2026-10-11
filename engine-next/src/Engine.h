#pragma once

#include "BunnyResult.h"
#include "Timer.h"
#include "Input.h"
#include "Window.h"

#include <memory>

namespace Bunny
{

class GraphicsSystem;
class Game;

class Engine
{
  public:
    BunnyResult initialize(std::unique_ptr<Game>&& game);
    BunnyResult runEngine();
    BunnyResult shutdown();

    ITTimer<double>& getTimer() { return mTimer; }
    Window& getWindow() { return *mWindow; }

  private:
    std::unique_ptr<Game> mGame;

    GameTimer<double> mTimer;
    std::unique_ptr<Window> mWindow;
    std::unique_ptr<InputManager> mInputManager;
    std::unique_ptr<GraphicsSystem> mGraphicsSystem;
};
} // namespace Bunny
