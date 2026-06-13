#pragma once

#include <cstdint>

namespace Bunny
{

using IdType = uint32_t;

//  wrapping this std::numeric_limits::max in parenthesis because windows defines min/max macro and will mess up things
//  here don't want to use /NOMINMAX
//  see https://stackoverflow.com/a/13566433
// static constexpr IdType BUNNY_INVALID_ID = (std::numeric_limits<IdType>::max)();
//  114514
static constexpr IdType BUNNY_INVALID_ID = 114514;

} // namespace Bunny