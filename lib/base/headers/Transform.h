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

    Transform() = default;
    explicit Transform(const glm::vec3& position, const glm::quat& rotationQuat, const glm::vec3& scale);
    explicit Transform(const glm::vec3& position, const glm::vec3& pitchYawRoll, const glm::vec3& scale);

    glm::mat4 mMatrix;
};
} // namespace Bunny::Base
