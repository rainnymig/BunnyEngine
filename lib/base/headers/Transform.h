#pragma once

#include <glm/vec3.hpp>

namespace Bunny::Base
{
struct Transform
{
    glm::vec3 mScale;
    glm::vec3 mRotation;
    glm::vec3 mPosition;
};
} // namespace Bunny::Base
