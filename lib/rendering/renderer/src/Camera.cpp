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

void Camera::getViewFrustum(ViewFrustum& outFrustum) const
{
    //  calculate the 6 planes of the camera view frustum

    //  near plane
    glm::vec3 nearCenter = mPosition + mForwardVec * NearPlaneDistance;
    outFrustum.mPlanes[0] = FrustumPlane(mForwardVec, nearCenter);
    //  far plane
    glm::vec3 farCenter = mPosition + mForwardVec * FarPlaneDistance;
    outFrustum.mPlanes[1] = FrustumPlane(-mForwardVec, farCenter);

    //  n: near, f: far,
    //  l: left, r: right, t: top, b: bottom, m: middle
    //  w: width, h: height
    //  e.g. ptln: point at top left corner of near plane
    float heightToDist = glm::tan(mFov / 2);
    float nh = heightToDist * NearPlaneDistance;
    float nw = nh * mAspectRatio;
    float fh = heightToDist * FarPlaneDistance;
    float fw = fh * mAspectRatio;

    glm::vec3 pmrn = nearCenter + mRightVec * nw;
    glm::vec3 pmln = nearCenter - mRightVec * nw;
    glm::vec3 ptmn = nearCenter + mUpVec * nh;
    glm::vec3 pbmn = nearCenter - mUpVec * nh;

    //  the calculation can actually be simplified, here we do it the full way to be clearer
    glm::vec3 vr = pmrn - mPosition;
    glm::vec3 vl = pmln - mPosition;
    glm::vec3 vt = ptmn - mPosition;
    glm::vec3 vb = pbmn - mPosition;

    //  left plane
    outFrustum.mPlanes[2] = FrustumPlane(glm::normalize(glm::cross(vl, mUpVec)), pmln);
    //  right plane
    outFrustum.mPlanes[3] = FrustumPlane(glm::normalize(glm::cross(mUpVec, vr)), pmrn);
    //  up plane
    outFrustum.mPlanes[4] = FrustumPlane(glm::normalize(glm::cross(vt, mRightVec)), ptmn);
    //  bottom plane
    outFrustum.mPlanes[5] = FrustumPlane(glm::normalize(glm::cross(mRightVec, vb)), pbmn);

    //  calculate data for occlusion culling
    outFrustum.mViewMat = getViewMatrix();
    outFrustum.mProjMat = getProjMatrix();
    outFrustum.mZNear = NearPlaneDistance;
    //  depth image width and height are filled elsewhere
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

    mForwardVec = rm * StaticForward;
    mUpVec = rm * StaticUp;
    mRightVec = rm * StaticRight;
}

FrustumPlane::FrustumPlane(const glm::vec3& normal, const glm::vec3 pointOnPlane)
    : mNormal(normal),
      mDistToOrigin(glm::dot(pointOnPlane, normal))
{
}
} // namespace Bunny::Render
