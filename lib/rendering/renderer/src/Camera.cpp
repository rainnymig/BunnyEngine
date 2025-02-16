#include "Camera.h"

namespace Bunny::Render
{
Camera::Camera(const glm::vec3& lookFrom, const glm::vec3& lookAt, float fov, float aspectRatio)
{
    mPosition = lookFrom;
    mViewMatrix = glm::lookAt(lookFrom, lookAt, {0, 0, 1});
    mProjMatrix = glm::perspective(fov, aspectRatio, NearPlaneDistance, FarPlaneDistance);
    mViewProjMatrix = mProjMatrix * mViewMatrix;
}
} // namespace Bunny::Render
