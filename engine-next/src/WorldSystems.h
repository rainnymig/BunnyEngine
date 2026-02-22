#pragma once

#include "Input.h"

#include "glm/common.hpp"
#include "glm/trigonometric.hpp"
#include "glm/vec3.hpp"

#include <string>

namespace Bunny::Engine
{

class World;

class CameraSystem
{
  public:
    CameraSystem(Base::InputManager* inputManager);
    void update(World* world, float deltaTime);

  private:
    void onKeyboardInput(const std::string& keyName, Base::InputManager::KeyState state);
    void showImguiControlPanel();

    float mMoveVelocity = 10;
    float mRotateVelocity = 45.0f;

    glm::vec3 mMoveVector{0, 0, 0};
    glm::vec3 mRotateVector{0, 0, 0};
};

class ObjectRandomMovementSystem
{
  public:
    void update(World* world, float deltaTime, float time);
};

} // namespace Bunny::Engine
