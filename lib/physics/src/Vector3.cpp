#include "Vector3.h"

#include <cmath>

namespace Bunny::Physics
{
constexpr Vector3::Vector3() noexcept : Vector3(0, 0, 0)
{
}

constexpr Vector3::Vector3(Real x, Real y, Real z) noexcept : x(x), y(y), z(z)
{
}

inline constexpr void Vector3::invert() noexcept
{
    x = -x;
    y = -y;
    z = -z;
}

inline Real Vector3::length() const
{
    return std::sqrt(x * x + y * y + z * z);
}

inline constexpr Real Vector3::lengthSquared() const noexcept
{
    return x * x + y * y + z * z;
}

inline void Vector3::normalize()
{
    Real l = length();
    if (l > 0)
    {
        (*this) = (*this) * ((Real)1 / l);
    }
}

} // namespace Bunny::Physics