#include "Material.h"

#include "Shader.h"
#include "ErrorCheck.h"
#include "Vertex.h"
#include "PipelineBuilder.h"

#include <cassert>

namespace Bunny::Render
{
void BasicBlinnPhongMaterial::buildPipeline(VkDevice device, VkFormat colorAttachmentFormat, VkFormat depthFormat)
{
    assert(device != nullptr);

    mDevice = device;

    //  load shader file
    Shader vertexShader(VERTEX_SHADER_PATH, device);
    Shader fragmentShader(FRAGMENT_SHADER_PATH, device);

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

    VK_HARD_CHECK(vkCreatePipelineLayout(mDevice, &pipelineLayoutInfo, nullptr, &mPipeline.mPipelineLayout));
    
    //  vertex info
    auto bindingDescription = getBindingDescription<NormalVertex>(0, VertexInputRate::Vertex);
    auto attributeDescriptions = NormalVertex::getAttributeDescriptions();

    PipelineBuilder builder;
    builder.setShaders(vertexShader.getShaderModule(), fragmentShader.getShaderModule());
    builder.setVertexInput(attributeDescriptions.data(), attributeDescriptions.size(), &bindingDescription, 1);
    builder.setInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    builder.setPolygonMode(VK_POLYGON_MODE_FILL);
    builder.setCulling(VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE);
    builder.setMultisamplingNone();
    builder.disableBlending(); //  opaque pipeline
    builder.enableDepthTest(VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL);
    builder.setColorAttachmentFormat(colorAttachmentFormat);
    builder.setDepthFormat(depthFormat);
    builder.setPipelineLayout(mPipeline.mPipelineLayout);

    mPipeline.mPipiline = builder.build(mDevice);

    //  init descriptor allocator
    constexpr size_t MAX_SETS = 2; //   should be max frames inflight
    DescriptorAllocator::PoolSize poolSizes[] = {
        {.mType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         .mRatio = 2},
    };
    mDescriptorAllocator.Init(mDevice, MAX_SETS, poolSizes);
}

void BasicBlinnPhongMaterial::cleanupPipeline()
{
    if (mDevice == nullptr)
    {
        return;
    }

    mDescriptorAllocator.DestroyPools(mDevice);
    vkDestroyPipeline(mDevice, mPipeline.mPipiline, nullptr);
    vkDestroyDescriptorSetLayout(mDevice, mDescriptorSetLayout, nullptr);
}

MaterialInstance BasicBlinnPhongMaterial::createInstance()
{
    MaterialInstance newInstance;

    newInstance.mpBaseMaterial = &mPipeline;
    mDescriptorAllocator.Allocate(mDevice, &mDescriptorSetLayout, &newInstance.mDescriptorSet);

    return newInstance;
}

void BasicBlinnPhongMaterial::buildDescriptorSetLayout(VkDevice device)
{
    DescriptorLayoutBuilder builder;

    //  Note: currently both bindings are in the same set
    //  might consider spliting them into two sets
    //  so one can be bound at scene level (mvp matrix)
    //  and here only deal with material stuff (lighting, texture)

    //  NOTE AGAIN !!IMPORTANT!!: here includes descriptors for both scene and material related stuff
    //  this is needed because the whole graphics pipeline is created here
    //  but when actually allocating desc sets material should only allocate material desc sets
    //  scene desc sets should be allocated outside (maybe in scene)
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
