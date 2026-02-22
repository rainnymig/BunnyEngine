#include "Transform.h"
#include "Transform.h"

#include <glm/gtx/euler_angles.hpp>

namespace Bunny::Base
{
Transform::Transform(const glm::vec3& position, const glm::quat& rotationQuat, const glm::vec3& scale)
{
    glm::mat4 tm = glm::translate(glm::mat4(1.f), position);
    glm::mat4 rm = glm::toMat4(rotationQuat);
    glm::mat4 sm = glm::scale(glm::mat4(1.f), scale);

    mMatrix = tm * rm * sm;
    mScale = scale;
}

Transform::Transform(const glm::vec3& position, const glm::vec3& pitchYawRoll, const glm::vec3& scale)
{
    glm::mat4 tm = glm::translate(glm::mat4(1.f), position);
    glm::mat4 rm = glm::eulerAngleYXZ(pitchYawRoll.y, pitchYawRoll.x, pitchYawRoll.z);
    glm::mat4 sm = glm::scale(glm::mat4(1.f), scale);

    mMatrix = tm * rm * sm;
    mScale = scale;
}

Transform::Transform(const glm::mat4& matrix)
{
    mMatrix = matrix;
    mScale = glm::vec3(
        glm::length(glm::vec3(matrix[0])), glm::length(glm::vec3(matrix[1])), glm::length(glm::vec3(matrix[2])));
}

} // namespace Bunny::Base
