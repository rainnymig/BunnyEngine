#include "Material.h"

#include "Shader.h"

#include <cassert>

namespace Bunny::Render
{
void BasicBlinnPhongMaterial::buildPipeline(VkDevice device)
{
    assert(device != nullptr);

    mDevice = device;

    //  load shader file
    Shader vertShader(VERTEX_SHADER_PATH, device);
    Shader fragShader(FRAGMENT_SHADER_PATH, device);

    //  build pipeline
    //  build descriptor set layout
    buildDescriptorSetLayout(device);

    //  pipeline layout
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;                  // Optional
    pipelineLayoutInfo.pSetLayouts = &mDescriptorSetLayout; // Optional
    pipelineLayoutInfo.pushConstantRangeCount = 0;          // Optional
    pipelineLayoutInfo.pPushConstantRanges = nullptr;       // Optional

    //  init descriptor allocator
}

void BasicBlinnPhongMaterial::cleanupPipeline()
{
    if (mDevice == nullptr)
    {
        return;
    }

    vkDestroyDescriptorSetLayout(mDevice, mDescriptorSetLayout, nullptr);
}

void BasicBlinnPhongMaterial::buildDescriptorSetLayout(VkDevice device)
{
    DescriptorLayoutBuilder builder;

    //  Note: currently both bindings are in the same set
    //  might consider spliting them into two sets
    //  so one can be bound at scene level (mvp matrix)
    //  and here only deal with material stuff (lighting, texture)
    {
        VkDescriptorSetLayoutBinding uniformBufferLayout{};
        uniformBufferLayout.binding = 0;
        uniformBufferLayout.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uniformBufferLayout.descriptorCount = 1;
        uniformBufferLayout.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        uniformBufferLayout.pImmutableSamplers = nullptr;
        builder.AddBinding(uniformBufferLayout);
    }
    {
        VkDescriptorSetLayoutBinding uniformBufferLayout{};
        uniformBufferLayout.binding = 1;
        uniformBufferLayout.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uniformBufferLayout.descriptorCount = 1;
        uniformBufferLayout.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        uniformBufferLayout.pImmutableSamplers = nullptr;
        builder.AddBinding(uniformBufferLayout);
    }

    mDescriptorSetLayout = builder.Build(mDevice);
}

} // namespace Bunny::Render
