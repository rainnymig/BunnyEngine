#pragma once

#include "Vector.h"

namespace Bunny::Physics
{
class MassPoint
{
  public:
    //  update the particle's kinematics state
    void update(float duration);

  private:
    //  get amount of damping applied to linear motion
    //  this is to reduce numeric instability
    constexpr static float getLinearMotionDamping();

    Vector3 mPosition;
    Vector3 mVelocity;
    Vector3 mAcceleration;

    //  holds inverse mass because it's easier to model infinite mass
    float mInverseMass;
};
} // namespace Bunny::Physics