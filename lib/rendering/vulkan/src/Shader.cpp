#include "Shader.h"

#include "Error.h"

#include <fstream>

namespace Bunny::Render
{
Shader::Shader(std::string_view shaderPath, VkDevice device) : mDevice(device)
{
    std::vector<std::byte> fileBuffer = Shader::readShaderFile(shaderPath);
    createShaderModule(fileBuffer, device);
}

Shader::~Shader()
{
    destroyShaderModule();
}

std::vector<std::byte> Shader::readShaderFile(std::string_view path)
{
    std::vector<std::byte> buffer;

    std::ifstream shaderFile(path.data(), std::ios::ate | std::ios::binary);
    if (!shaderFile.is_open())
    {
        PRINT_AND_RETURN_VALUE("Can not open shader file!", buffer);
    }

    size_t fileSize = (size_t)shaderFile.tellg();
    buffer.resize(fileSize);
    shaderFile.seekg(0);
    shaderFile.read((char*)buffer.data(), fileSize);

    shaderFile.close();
    return buffer;
}

void Shader::createShaderModule(const std::vector<std::byte>& code, VkDevice device)
{
    if (code.empty()) 
    {
        PRINT_AND_RETURN("Shader code is empty!")
    }

    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule shaderModule = nullptr;
    if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
    {
        PRINT_AND_RETURN("failed to create shader module!")
    }

    mShaderModule = shaderModule;
}
void Shader::destroyShaderModule()
{
    if (mDevice != nullptr && mShaderModule != nullptr)
    {
        vkDestroyShaderModule(mDevice, mShaderModule, nullptr);
        mShaderModule = nullptr;
    }
}
} // namespace Bunny::Render