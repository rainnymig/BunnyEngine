#pragma once

#include <glm/vec3.hpp>

namespace Bunny::Render
{
struct DirectionalLight
{
    glm::vec3 mDirection;
    glm::vec3 mColor;
};
} // namespace Bunny::Render
