#pragma once

#include "Transform.h"

#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>

#include <vector>

namespace Bunny::Render
{

    struct BoundingSphere
    {
        glm::vec3 mCenter;
        float mRadius;
    };

    struct CullComponenet
    {
        Base::Transform mTransform;
        BoundingSphere mBounds;
    };

    class CullingPass
    {
        void Cull(const std::vector<CullComponenet>& comps);
    };
} // namespace Bunny::Render
