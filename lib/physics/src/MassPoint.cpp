#include "MassPoint.h"

#include <cassert>
#include <cmath>

void Bunny::Physics::MassPoint::update(float duration)
{
    //  no need to update if mass is infinite
    if (mInverseMass == 0)
    {
        return;
    }

    assert(duration > 0);

    mPosition += mVelocity * duration;

    Vector3 resultingAcceleration = mAcceleration;

    mVelocity += resultingAcceleration * duration;

    //  add damping to reduce numeric instability
    mVelocity = mVelocity * std::pow(getLinearMotionDamping(), duration);
}

constexpr float Bunny::Physics::MassPoint::getLinearMotionDamping()
{
    return 0.999f;
}
