#pragma once

#include <concepts>
#include <cassert>

namespace Bunny::Base
{
//  taken from https://github.com/nvpro-samples/nvpro_core/blob/master/nvh/alignment.hpp

//  rounds x up to a multiple of a. a must be a power of two.
template <std::integral IntegralType>
constexpr IntegralType alignUp(IntegralType x, size_t a)
{
    assert(a % 2 == 0);
    return IntegralType((x + (IntegralType(a) - 1)) & ~IntegralType(a - 1));
}

//  rounds x down to a multiple of a. a must be a power of two.
template <std::integral IntegralType>
constexpr IntegralType alignDown(IntegralType x, size_t a)
{
    assert(a % 2 == 0);
    return IntegralType(x & ~IntegralType(a - 1));
}

} // namespace Bunny::Base
