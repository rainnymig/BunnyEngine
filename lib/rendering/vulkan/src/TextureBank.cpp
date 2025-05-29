#include "TextureBank.h"

#include "VulkanRenderResources.h"
#include "VulkanGraphicsRenderer.h"
#include "Error.h"

#include <stb_image.h>
#include <fmt/core.h>

namespace Bunny::Render
{
TextureBank::TextureBank(const VulkanRenderResources* vulkanResources, const VulkanGraphicsRenderer* renderer)
    : mVulkanResources(vulkanResources),
      mRenderer(renderer)
{
}

BunnyResult TextureBank::addTexture(const char* filePath, VkFormat format, IdType& outId)
{
    outId = BUNNY_INVALID_ID;

    int texWidth;
    int texHeight;
    int texChannels;
    int desiredChannels = STBI_rgb_alpha;
    stbi_uc* texData = stbi_load(filePath, &texWidth, &texHeight, &texChannels, desiredChannels);
    if (texData == nullptr)
    {
        std::string errMsg =
            fmt::format("Can not load image from {} because of stbi error {}", filePath, stbi_failure_reason());
        PRINT_AND_RETURN_VALUE(errMsg, BUNNY_SAD);
    }

    AllocatedImage texture;
    VkDeviceSize dataSize = texWidth * texHeight * desiredChannels;
    BUNNY_CHECK_SUCCESS_OR_RETURN_RESULT(mVulkanResources->createImageWithData(static_cast<void*>(texData), dataSize,
        VkExtent3D{static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight), 1}, format,
        VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, texture))

    outId = mTextures.size();
    mTextures.push_back(texture);

    return BUNNY_HAPPY;
}

void TextureBank::cleanup()
{
    for (AllocatedImage& texture : mTextures)
    {
        mVulkanResources->destroyImage(texture);
    }
}
} // namespace Bunny::Render
