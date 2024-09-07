#include "Helper.h"

namespace Bunny::Render
{
std::vector<std::byte> readShaderFile(std::string_view path)
{
    std::ifstream shaderFile(path.data(), std::ios::ate | std::ios::binary);
    if (!shaderFile.is_open())
    {
        throw std::runtime_error("can't open shader file.");
    }

    size_t fileSize = (size_t)shaderFile.tellg();
    std::vector<std::byte> buffer(fileSize);
    shaderFile.seekg(0);
    shaderFile.read((char*)buffer.data(), fileSize);

    shaderFile.close();
    return buffer;
}
}