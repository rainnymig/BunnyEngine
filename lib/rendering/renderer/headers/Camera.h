#pragma once

#include <Transform.h>

#include <glm/trigonometric.hpp>
#include <glm/mat4x4.hpp>

namespace Bunny::Render
{

class Camera
{
  public:
    //  fov angle is in vertical direction, is radians
    Camera(const glm::vec3& lookFrom = {0, 5, 10}, const glm::vec3& lookAt = {0, 0, 0}, float fov = glm::radians(45.0f),
        float aspectRatio = 16.0f / 9.0f);

    glm::mat4 getViewMatrix() const { return mViewMatrix; }
    glm::mat4 getProjMatrix() const { return mProjMatrix; }
    glm::mat4 getViewProjMatrix() const { return mViewProjMatrix; }
    glm::vec3 getPosition() const { return mPosition; }

    static constexpr float NearPlaneDistance = 0.1f;
    static constexpr float FarPlaneDistance = 100.0f;

  private:
    glm::mat4 mViewMatrix;
    glm::mat4 mProjMatrix;
    glm::mat4 mViewProjMatrix;
    glm::vec3 mPosition;
};

} // namespace Bunny::Render
