#include "Camera.h"

#include <glm/gtx/euler_angles.hpp>

namespace Bunny::Render
{
Camera::Camera(const glm::vec3& lookFrom, const glm::vec3& pitchYawRoll, float fov, float aspectRatio)
{
    mPosition = lookFrom;
    mPitchYawRoll = pitchYawRoll;
    mFov = fov;
    mAspectRatio = aspectRatio;

    updateMatrices();
}

void Camera::setPosition(const glm::vec3& position)
{
    mPosition = position;
    updateMatrices();
}

void Camera::setRotation(const glm::vec3& pitchYawRoll)
{
    mPitchYawRoll = pitchYawRoll;
    updateMatrices();
}

void Camera::setAspectRatio(float ratio)
{
    mAspectRatio = ratio;
    updateMatrices();
}

void Camera::updateMatrices()
{
    glm::mat4 tm = glm::translate(glm::mat4(1.f), mPosition);
    glm::mat4 rm = glm::eulerAngleYXZ(mPitchYawRoll.y, mPitchYawRoll.x, mPitchYawRoll.z);
    mViewMatrix = glm::inverse(tm * rm);

    mProjMatrix = glm::perspective(mFov, mAspectRatio, NearPlaneDistance, FarPlaneDistance);
    //  inverse Y direction to make it point upwards, ref:
    //  https://matthewwellings.com/blog/the-new-vulkan-coordinate-system/
    mProjMatrix[1][1] *= -1;
    mViewProjMatrix = mProjMatrix * mViewMatrix;
}
} // namespace Bunny::Render
