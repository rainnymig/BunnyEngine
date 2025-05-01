#pragma once

#include <glm/vec3.hpp>
#include <glm/matrix.hpp>
#include <glm/gtx/quaternion.hpp>

namespace Bunny::Base
{
struct Transform
{
    Transform() = default;
    explicit Transform(const glm::vec3& position, const glm::quat& rotationQuat, const glm::vec3& scale);
    explicit Transform(const glm::vec3& position, const glm::vec3& pitchYawRoll, const glm::vec3& scale);

    glm::mat4 mMatrix;
    glm::vec3 mScale;
};
} // namespace Bunny::Base
