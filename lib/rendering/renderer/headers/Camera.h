#pragma once

#include <Transform.h>

#include <glm/trigonometric.hpp>
#include <glm/matrix.hpp>

namespace Bunny::Render
{

struct FrustumPlane
{
    FrustumPlane() = default;
    FrustumPlane(const glm::vec3& normal, const glm::vec3 pointOnPlane);

    glm::vec3 mNormal;
    float mDistToOrigin;
};

struct ViewFrustum
{
    FrustumPlane mPlanes[6];

    glm::mat4 mViewMat;
    glm::mat4 mProjMat;
    float mZNear;
    float mDepthImageWidth;
    float mDepthImageHeight;
};

class Camera
{
  public:
    //  fov angle is in vertical direction, is radians
    Camera(const glm::vec3& position = {0, 0, 10}, const glm::vec3& pitchYawRoll = {0, 0, 0},
        float fov = glm::radians(45.0f), float aspectRatio = 16.0f / 9.0f);
    virtual ~Camera() = default;

    void setPosition(const glm::vec3& position);
    void setRotation(const glm::vec3& pitchYawRoll);
    void setDeltaPosition(const glm::vec3& deltaPos);
    void setDeltaRotation(const glm::vec3& deltaPitchYawRoll);

    void setAspectRatio(float ratio);

    glm::mat4 getViewMatrix() const { return mViewMatrix; }
    glm::mat4 getProjMatrix() const { return mProjMatrix; }
    glm::mat4 getViewProjMatrix() const { return mViewProjMatrix; }
    glm::vec3 getPosition() const { return mPosition; }
    glm::vec3 getRotation() const { return mPitchYawRoll; }
    void getViewFrustum(ViewFrustum& outFrustum) const;

  protected:
    void updateMatrices();

    static constexpr float NearPlaneDistance = 0.1f;
    static constexpr float FarPlaneDistance = 100.0f;
    static constexpr glm::vec4 StaticForward{0, 0, -1, 0};
    static constexpr glm::vec4 StaticUp{0, 1, 0, 0};
    static constexpr glm::vec4 StaticRight{1, 0, 0, 0};

    glm::mat4 mViewMatrix;
    glm::mat4 mProjMatrix;
    glm::mat4 mViewProjMatrix;
    glm::vec3 mPosition;
    glm::vec3 mPitchYawRoll;
    glm::vec3 mForwardVec = StaticForward;
    glm::vec3 mUpVec = StaticUp;
    glm::vec3 mRightVec = StaticRight;
    float mFov;
    float mAspectRatio; //  width/height
};

class PhysicalCamera : public Camera
{
  public:
    virtual ~PhysicalCamera() override = default;

    float getExposure() const;

    void setAperture(float apurture) { mAperture = mAperture; }
    void setShutterTime(float shutterTime) { mShutterTime = shutterTime; }
    void setIso(float iso) { mIso = iso; }

  protected:
    float mAperture = 2.8f;
    float mShutterTime = 0.01f;
    float mIso = 100;
};

} // namespace Bunny::Render
