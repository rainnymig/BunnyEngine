#pragma once

#include <glm/vec3.hpp>

namespace Bunny::Base
{
    struct BoundingSphere
    {
        glm::vec3 mCenter;
        float mRadius;
    };
} // namespace Bunny::Base
