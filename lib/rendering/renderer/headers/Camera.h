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
    Camera(const glm::vec3& lookFrom = {0, 0, 0}, const glm::vec3& lookAt = {0, 0, 0}, float fov = glm::radians(45.0f),
        float aspectRatio = 16.0f / 9.0f);

    inline glm::mat4 getViewProjMatrix() const;

    static constexpr float NearPlaneDistance = 0.1f;
    static constexpr float FarPlaneDistance = 100.0f;

  private:
    glm::mat4 mViewProjMatrix;
};

} // namespace Bunny::Render
