#include "WorldSystems.h"

#include "World.h"
#include "Camera.h"
#include "WorldComponents.h"
#include "Input.h"

#include <entt/entt.hpp>

namespace Bunny::Engine
{
Engine::CameraSystem::CameraSystem(Base::InputManager* inputManager)
{
    inputManager->registerKeyboardCallback(
        [this](const std::string& keyName, Base::InputManager::KeyState state) { onKeyboardInput(keyName, state); });
}

void Engine::CameraSystem::update(World* world, float deltaTime)
{
    const auto camComps = world->mEntityRegistry.view<PbrCameraComponent>();
    if (!camComps.empty())
    {
        auto& cam = world->mEntityRegistry.get<PbrCameraComponent>(camComps.front());
        Render::Camera& camera = cam.mCamera;
        // camera.setDeltaRotation(glm::vec3(0, deltaTime * glm::pi<double>() / 16, 0));
        camera.setDeltaPosition(mMoveVector * mMoveVelocity * deltaTime);
    }
}

void Engine::CameraSystem::onKeyboardInput(const std::string& keyName, Base::InputManager::KeyState state)
{
    if (state == Base::InputManager::KeyState::Press)
    {
        if (keyName == "w")
        {
            mMoveVector.z = 1;
        }
        else if (keyName == "s")
        {
            mMoveVector.z = -1;
        }
        else if (keyName == "a")
        {
            mMoveVector.x = -1;
        }
        else if (keyName == "d")
        {
            mMoveVector.x = 1;
        }
        else if (keyName == "e")
        {
            mMoveVector.y = 1;
        }
        else if (keyName == "c")
        {
            mMoveVector.y = -1;
        }
    }
    else if (state == Base::InputManager::KeyState::Release)
    {
        if (keyName == "w" || keyName == "s")
        {
            mMoveVector.z = 0;
        }
        else if (keyName == "a" || keyName == "d")
        {
            mMoveVector.x = 0;
        }
        else if (keyName == "e" || keyName == "c")
        {
            mMoveVector.y = 0;
        }
    }
}

} // namespace Bunny::Engine
