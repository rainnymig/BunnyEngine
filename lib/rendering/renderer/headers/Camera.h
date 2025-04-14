#pragma once

#include <Transform.h>

#include <glm/trigonometric.hpp>
#include <glm/matrix.hpp>

namespace Bunny::Render
{

class Camera
{
  public:
    //  fov angle is in vertical direction, is radians
    Camera(const glm::vec3& lookFrom = {0, 0, 10}, const glm::vec3& pitchYawRoll = {0, 0, 0},
        float fov = glm::radians(45.0f), float aspectRatio = 16.0f / 9.0f);

    void setPosition(const glm::vec3& position);
    void setRotation(const glm::vec3& pitchYawRoll);

    void setAspectRatio(float ratio);

    glm::mat4 getViewMatrix() const { return mViewMatrix; }
    glm::mat4 getProjMatrix() const { return mProjMatrix; }
    glm::mat4 getViewProjMatrix() const { return mViewProjMatrix; }
    glm::vec3 getPosition() const { return mPosition; }

    static constexpr float NearPlaneDistance = 0.1f;
    static constexpr float FarPlaneDistance = 100.0f;

  private:
    void updateMatrices();

    glm::mat4 mViewMatrix;
    glm::mat4 mProjMatrix;
    glm::mat4 mViewProjMatrix;
    glm::vec3 mPosition;
    glm::vec3 mPitchYawRoll;
    float mFov;
    float mAspectRatio;
};

} // namespace Bunny::Render
