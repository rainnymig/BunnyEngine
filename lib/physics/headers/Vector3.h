#pragma once

#include "Common.h"
#include <concepts>
#include <type_traits>

namespace Bunny::Physics
{
class alignas(sizeof(Real) * 4) Vector3
{
  public:
    constexpr Vector3() noexcept;
    constexpr Vector3(Real x, Real y, Real z) noexcept;
    constexpr Vector3(const Vector3& other) noexcept = default;
    constexpr Vector3(Vector3&& other) noexcept = default;
    constexpr Vector3& operator=(const Vector3& other) noexcept = default;
    constexpr Vector3& operator=(Vector3&& other) noexcept = default;

    inline constexpr void invert() noexcept;
    inline Real length() const;
    inline constexpr Real lengthSquared() const noexcept;
    inline void normalize();

    Real x;
    Real y;
    Real z;
};

template <typename S>
    requires std::is_arithmetic_v<S>
constexpr Vector3 operator*(const Vector3& v, S s) noexcept
{
    return Vector3(v.x * s, v.y * s, v.z * s);
}

} // namespace Bunny::Physics