#pragma once

#include <glm/vec3.hpp>

namespace Bunny::Render
{
struct DirectionalLight
{
    glm::vec3 mDirection;
    float mPadding1;
    glm::vec3 mColor;
    float mPadding2;
};
} // namespace Bunny::Render
