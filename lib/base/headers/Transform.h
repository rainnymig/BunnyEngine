#pragma once

#include <glm/vec3.hpp>
#include <glm/gtx/quaternion.hpp>

namespace Bunny::Base
{
struct Transform
{
    glm::vec3 mScale;
    glm::quat mRotation;
    glm::vec3 mPosition;
};
} // namespace Bunny::Base
