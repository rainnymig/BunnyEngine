#pragma once

#include "Transform.h"

#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>

#include <vector>

namespace Bunny::Render
{
class CullingPass
{
    CullingPass();

    void initializePass();
    void cleanup();
};
} // namespace Bunny::Render
