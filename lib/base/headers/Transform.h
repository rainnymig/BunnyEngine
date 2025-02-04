#pragma once

#include <glm/vec3.hpp>
#include <glm/matrix.hpp>
#include <glm/gtx/quaternion.hpp>

namespace Bunny::Base
{
struct Transform
{
    // glm::vec3 mScale;
    // glm::quat mRotation;
    // glm::vec3 mPosition;

    glm::mat4 mMatrix;
};
} // namespace Bunny::Base
