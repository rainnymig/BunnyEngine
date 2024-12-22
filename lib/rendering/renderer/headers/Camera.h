#pragma once

#include <Transform.h>

#include <glm/trigonometric.hpp>

namespace Bunny::Render
{

class Camera
{
  public:
    Camera(const glm::vec3& position = {0, 0, 0}, const glm::vec3& rotation = {0, 0, 0},
        float fov = glm::radians(45.0f), float aspectRatio = 16.0f / 9.0f);

  private:
    Base::Transform mTransform;
    float mFov; //  field of view, in radians
    float mAspectRatio;
};

} // namespace Bunny::Render
