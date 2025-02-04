#include "Camera.h"

namespace Bunny::Render
{
Camera::Camera(const glm::vec3& lookFrom, const glm::vec3& lookAt, float fov, float aspectRatio)
{
    glm::mat4 viewMat = glm::lookAt(lookFrom, lookAt, {0, 0, 1});
    glm::mat4 projMat = glm::perspective(fov, aspectRatio, NearPlaneDistance, FarPlaneDistance);

    mViewProjMatrix = projMat * viewMat;
}

inline glm::mat4 Camera::getViewProjMatrix() const
{
    return mViewProjMatrix;
}

} // namespace Bunny::Render
