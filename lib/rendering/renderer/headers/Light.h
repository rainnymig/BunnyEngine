#pragma once

#include <glm/vec3.hpp>

namespace Bunny::Render
{
struct DirectionalLight
{
    glm::vec3 mDirection;
    float mPad1;
    glm::vec3 mColor;
    float mPad2;
};
} // namespace Bunny::Render
