#pragma once

#include <vulkan/vulkan.h>

#include <string_view>
#include <vector>

namespace Bunny::Render
{
class Shader
{
  public:
    Shader(std::string_view shaderPath, VkDevice device);
    ~Shader();

    VkShaderModule getShaderModule() const { return mShaderModule; }

  private:
    static std::vector<std::byte> readShaderFile(std::string_view path);
    void createShaderModule(const std::vector<std::byte>& code, VkDevice device);
    void destroyShaderModule();

    VkShaderModule mShaderModule = nullptr;
    VkDevice mDevice = nullptr;
};
} // namespace Bunny::Render