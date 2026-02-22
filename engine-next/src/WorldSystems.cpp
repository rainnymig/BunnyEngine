#include "WorldSystems.h"

#include "World.h"
#include "Camera.h"
#include "WorldComponents.h"
#include "ImguiHelper.h"

#include <imgui.h>
#include <entt/entt.hpp>

namespace Bunny::Engine
{
Engine::CameraSystem::CameraSystem(Base::InputManager* inputManager)
{
    inputManager->registerKeyboardCallback(
        [this](const std::string& keyName, Base::InputManager::KeyState state) { onKeyboardInput(keyName, state); });
    Base::ImguiHelper::get().registerCommand([this]() { showImguiControlPanel(); });
}

void Engine::CameraSystem::update(World* world, float deltaTime)
{
    const auto camComps = world->mEntityRegistry.view<PbrCameraComponent>();
    if (!camComps.empty())
    {
        auto& cam = world->mEntityRegistry.get<PbrCameraComponent>(camComps.front());
        Render::Camera& camera = cam.mCamera;
        camera.recordPrevViewProjMatrix();

        // camera.setDeltaRotation(glm::vec3(0, deltaTime * glm::pi<double>() / 16, 0));
        camera.setDeltaPosition(mMoveVector * mMoveVelocity * deltaTime);
        camera.setDeltaRotation(mRotateVector * glm::radians(mRotateVelocity) * deltaTime);
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
        else if (keyName == "k")
        {
            mRotateVector.x = 1;
        }
        else if (keyName == "i")
        {
            mRotateVector.x = -1;
        }
        else if (keyName == "j")
        {
            mRotateVector.y = 1;
        }
        else if (keyName == "l")
        {
            mRotateVector.y = -1;
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
        else if (keyName == "i" || keyName == "k")
        {
            mRotateVector.x = 0;
        }
        else if (keyName == "j" || keyName == "l")
        {
            mRotateVector.y = 0;
        }
    }
}

void CameraSystem::showImguiControlPanel()
{
    ImGui::Begin("Camera and Lights");

    ImGui::Text("Camera Control");
    ImGui::DragFloat("Camera move speed", &mMoveVelocity, 1, 1, 1000, "%.1f");
    ImGui::DragFloat("Camera rotation speed", &mRotateVelocity, 1, 1, 90, "%.1f");
    ImGui::Separator();

    ImGui::End();
}

void Engine::ObjectRandomMovementSystem::update(World* world, float deltaTime, float time)
{
    static constexpr glm::vec3 maxTranslateVelocity{0, 3, 0};
    static constexpr float phaseInteval = 0.05f;
    auto meshComps = world->mEntityRegistry.view<TransformComponent>();
    for (auto [entity, transform] : meshComps.each())
    {
        // float phaseOffset = transform.mTransform.mMatrix[3][0] * phaseInteval;
        // transform.mTransform.mMatrix = glm::translate(
        //     transform.mTransform.mMatrix, maxTranslateVelocity * glm::sin(time + phaseOffset) * deltaTime);
        glm::mat4 rotMat =
            glm::rotate(glm::mat4(1.0f), glm::pi<float>() / 16.0f * deltaTime, glm::vec3(0.0f, 1.0f, 0.0f));
        transform.mTransform.mMatrix = transform.mTransform.mMatrix * rotMat;
    }
}

} // namespace Bunny::Engine
