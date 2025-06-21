#include "TextureBank.h"

#include "VulkanRenderResources.h"
#include "VulkanGraphicsRenderer.h"
#include "Error.h"
#include "ErrorCheck.h"
#include "Descriptor.h"

#include <stb_image.h>
#include <fmt/core.h>

#include <algorithm>
#include <iterator>
#include <cassert>

namespace Bunny::Render
{
TextureBank::TextureBank(const VulkanRenderResources* vulkanResources, const VulkanGraphicsRenderer* renderer)
    : mVulkanResources(vulkanResources),
      mRenderer(renderer)
{
}

BunnyResult TextureBank::initialize()
{
    BUNNY_CHECK_SUCCESS_OR_RETURN_RESULT(createSampler())
    return BUNNY_HAPPY;
}

BunnyResult TextureBank::addTexture(const char* filePath, VkFormat format, IdType& outId)
{
    outId = BUNNY_INVALID_ID;

    //  if the texture is already loaded, return directly
    auto iter = mTexturePathToIds.find(filePath);
    if (iter != mTexturePathToIds.end())
    {
        outId = iter->second;
        return BUNNY_HAPPY;
    }

    int texWidth;
    int texHeight;
    int texChannels;
    const int desiredChannels = STBI_default;
    stbi_uc* texData = stbi_load(filePath, &texWidth, &texHeight, &texChannels, desiredChannels);
    if (texData == nullptr)
    {
        std::string errMsg =
            fmt::format("Can not load image from {} because of stbi error {}", filePath, stbi_failure_reason());
        PRINT_AND_RETURN_VALUE(errMsg, BUNNY_SAD);
    }

    AllocatedImage texture;
    VkDeviceSize dataSize = texWidth * texHeight * texChannels;
    BUNNY_CHECK_SUCCESS_OR_RETURN_RESULT(mVulkanResources->createImageWithData(static_cast<void*>(texData), dataSize,
        VkExtent3D{static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight), 1}, format,
        VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, texture))

    outId = mTextures.size();
    mTextures.push_back(texture);
    mTexturePathToIds[filePath] = outId;

    stbi_image_free(texData);
    return BUNNY_HAPPY;
}

BunnyResult TextureBank::addDescriptorSetWrite(uint32_t binding, DescriptorWriter& outWriter) const
{
    if (mTextures.empty())
    {
        return BUNNY_HAPPY;
    }
    std::vector<VkDescriptorImageInfo> imageInfos;
    imageInfos.reserve(mTextures.size());
    std::transform(
        mTextures.begin(), mTextures.end(), std::back_inserter(imageInfos), [this](const AllocatedImage& tex) {
            return VkDescriptorImageInfo{.sampler = mImageSampler,
                .imageView = tex.mImageView,
                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
        });

    outWriter.writeImages(binding, std::move(imageInfos), VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

    return BUNNY_HAPPY;
}

void TextureBank::cleanup()
{
    for (AllocatedImage& texture : mTextures)
    {
        mVulkanResources->destroyImage(texture);
    }

    if (mImageSampler != nullptr)
    {
        vkDestroySampler(mVulkanResources->getDevice(), mImageSampler, nullptr);
    }
}

BunnyResult TextureBank::createSampler()
{
    VkSamplerCreateInfo createInfo = {};

    createInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    createInfo.magFilter = VK_FILTER_LINEAR;
    createInfo.minFilter = VK_FILTER_LINEAR;
    createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    createInfo.minLod = 0;
    createInfo.maxLod = 1;
    createInfo.pNext = nullptr;

    VK_CHECK_OR_RETURN_BUNNY_SAD(vkCreateSampler(mVulkanResources->getDevice(), &createInfo, 0, &mImageSampler))

    return BUNNY_HAPPY;
}
} // namespace Bunny::Render
