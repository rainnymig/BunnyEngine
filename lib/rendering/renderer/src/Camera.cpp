#include "Camera.h"

namespace Bunny::Render
{
Camera::Camera(const glm::vec3& lookFrom, const glm::vec3& lookAt, float fov, float aspectRatio)
{
    mPosition = lookFrom;
    mViewMatrix = glm::lookAt(lookFrom, lookAt, {0, -1, 0});
    mProjMatrix = glm::perspective(fov, aspectRatio, NearPlaneDistance, FarPlaneDistance);
    // mProjMatrix[1][1] *= -1;
    mViewProjMatrix = mProjMatrix * mViewMatrix;
}
} // namespace Bunny::Render
