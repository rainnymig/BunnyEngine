#include "Camera.h"

namespace Bunny::Render
{
Camera::Camera(const glm::vec3& lookFrom, const glm::vec3& lookAt, float fov, float aspectRatio)
{
    mPosition = lookFrom;
    mLookAt = lookAt;
    mFov = fov;
    mAspectRatio = aspectRatio;

    updateMatrices();
}

void Camera::setAspectRatio(float ratio)
{
    mAspectRatio = ratio;
    updateMatrices();
}

void Camera::updateMatrices()
{
    mViewMatrix = glm::lookAt(mPosition, mLookAt, {0, -1, 0});
    mProjMatrix = glm::perspective(mFov, mAspectRatio, NearPlaneDistance, FarPlaneDistance);
    // mProjMatrix[1][1] *= -1;
    mViewProjMatrix = mProjMatrix * mViewMatrix;

}
} // namespace Bunny::Render
