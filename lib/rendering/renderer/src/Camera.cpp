#include "Camera.h"

namespace Bunny::Render
{
Camera::Camera(const glm::vec3& position, const glm::vec3& rotation, float fov, float aspectRatio)
    : mTransform{ .mScale={1.0f, 1.0f, 1.0f}, .mRotation = rotation, .mPosition = position}
    , mFov(fov)
    , mAspectRatio(aspectRatio)
{
}

} // namespace Bunny::Render
