#pragma once

#include <glm/vec3.hpp>

namespace Bunny::Physics
{

using Vector3 = glm::vec3;

// template <std::floating_point Real>
// class alignas(sizeof(Real) * 4) TVector3
// {
//   public:
//     constexpr TVector3(Real x, Real y, Real z) noexcept : x(x), y(y), z(z) {}
//     constexpr TVector3() noexcept : TVector3(0, 0, 0) {};
//     constexpr TVector3(const TVector3<Real>& other) noexcept = default;
//     constexpr TVector3(TVector3<Real>&& other) noexcept = default;
//     constexpr TVector3<Real>& operator=(const TVector3<Real>& other) noexcept = default;
//     constexpr TVector3<Real>& operator=(TVector3<Real>&& other) noexcept = default;

//     //  basic operations
//     constexpr TVector3<Real> operator-() const { return TVector3<Real>(-x, -y, -z); }
//     constexpr TVector3<Real> operator+(const TVector3<Real>& other) const
//     {
//         return TVector3<Real>(x + other.x, y + other.y, z + other.z);
//     }
//     constexpr TVector3<Real> operator-(const TVector3<Real>& other) const
//     {
//         return TVector3<Real>(x - other.x, y - other.y, z - other.z);
//     }

//     //  products
//     constexpr Real dot(const TVector3<Real>& other) const { return x * other.x + y * other.y + z * other.z; }
//     constexpr TVector3<Real> cross(const TVector3<Real>& other) const
//     {
//         return TVector3<Real>(y * other.z - z * other.y, z * other.x - x * other.z, x * other.y - y * other.x);
//     }

//     constexpr void invert() noexcept
//     {
//         x = -x;
//         y = -y;
//         z = -z;
//     }
//     constexpr Real length() const { return std::sqrt(x * x + y * y + z * z); }
//     constexpr Real lengthSquared() const noexcept { return x * x + y * y + z * z; }
//     constexpr void normalize()
//     {
//         Real l = length();
//         if (l > 0)
//         {
//             (*this) = (*this) * ((Real)1 / l);
//         }
//     }
//     constexpr void addScaled(const TVector3<Real>& other, Real scale)
//     {
//         x += other.x * scale;
//         y += other.y * scale;
//         z += other.z * scale;
//     }

//     Real x;
//     Real y;
//     Real z;
// };

// using Vector3 = TVector3<float>;
// using Vector3f = TVector3<float>;
// using Vector3d = TVector3<double>;

// template <std::floating_point Real, typename S>
//     requires std::is_arithmetic_v<S>
// constexpr TVector3<Real> operator*(const TVector3<Real>& v, S s)
// {
//     return TVector3<Real>(v.x * s, v.y * s, v.z * s);
// }

// template <std::floating_point Real, typename S>
//     requires std::is_arithmetic_v<S>
// constexpr TVector3<Real> operator/(const TVector3<Real>& v, S s)
// {
//     return TVector3<Real>(v.x / s, v.y / s, v.z / s);
// }

// template <std::floating_point Real, typename S>
//     requires std::is_arithmetic_v<S>
// constexpr TVector3<Real> operator+(const TVector3<Real>& v, S s)
// {
//     return TVector3<Real>(v.x + s, v.y + s, v.z + s);
// }

// template <std::floating_point Real, typename S>
//     requires std::is_arithmetic_v<S>
// constexpr TVector3<Real> operator-(const TVector3<Real>& v, S s)
// {
//     return TVector3<Real>(v.x - s, v.y - s, v.z - s);
// }

// template <std::floating_point Real>
// constexpr Real vDot(const TVector3<Real>& v1, const TVector3<Real>& v2)
// {
//     return v1.dot(v2);
// }

// template <std::floating_point Real>
// constexpr TVector3<Real> vCross(const TVector3<Real>& v1, const TVector3<Real>& v2)
// {
//     return v1.cross(v2);
// }

// template <std::floating_point Real>
// constexpr bool makeOrthonormalBasis(TVector3<Real>& va, TVector3<Real>& vb, TVector3<Real>& vc)
// {
//     va.normalize();
//     vc = vCross(va, vb);
//     if (vc.lengthSquared() == 0)
//     {
//         return false;
//     }
//     vc.normalize();
//     vb = vCross(vc, va);
//     return true;
// }

} // namespace Bunny::Physics