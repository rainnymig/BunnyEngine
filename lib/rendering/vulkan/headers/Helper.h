#pragma once

#include <vector>
#include <string_view>
#include <fstream>

namespace Bunny::Render
{
std::vector<std::byte> readShaderFile(std::string_view path);
} // namespace Bunny::Render