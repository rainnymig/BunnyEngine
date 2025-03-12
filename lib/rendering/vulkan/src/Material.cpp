#include "Material.h"

#include "Shader.h"
#include "ErrorCheck.h"
#include "Vertex.h"
#include "PipelineBuilder.h"

#include <cassert>

namespace Bunny::Render
{
BasicBlinnPhongMaterial::BasicBlinnPhongMaterial(Base::BunnyGuard<Builder> guard, VkDevice device)
    : mDevice(device)
{
    mId = std::hash<std::string_view>{}(getName());
}

void BasicBlinnPhongMaterial::cleanup()
{
    cleanupPipeline();
}

void BasicBlinnPhongMaterial::cleanupPipeline()
{
    if (mDevice == nullptr)
    {
        return;
    }

    // mDescriptorAllocator.destroyPools(mDevice);
    vkDestroyPipeline(mDevice, mPipeline.mPipeline, nullptr);
    vkDestroyPipelineLayout(mDevice, mPipeline.mPipelineLayout, nullptr);
    // vkDestroyDescriptorSetLayout(mDevice, mDescriptorSetLayout, nullptr);
}

MaterialInstance BasicBlinnPhongMaterial::makeInstance()
{
    //  Note: maybe create instance cache instead of recreating every time

    static int instanceIdCounter = 1;

    MaterialInstance newInstance;

    newInstance.mpBaseMaterial = &mPipeline;
    // mDescriptorAllocator.allocate(mDevice, &mDescriptorSetLayout, &newInstance.mDescriptorSet);

    std::string instanceName = fmt::format("{}_inst_{}", getName(), instanceIdCounter++);
    newInstance.mId = std::hash<std::string>{}(instanceName);

    return newInstance;
}

// void BasicBlinnPhongMaterial::buildDescriptorSetLayout(VkDevice device)
// {
//     DescriptorLayoutBuilder builder;

//     //  Note: currently both bindings are in the same set
//     //  might consider spliting them into two sets
//     //  so one can be bound at scene level (mvp matrix)
//     //  and here only deal with material stuff (lighting, texture)

//     //  NOTE AGAIN !!IMPORTANT!!: here includes descriptors for both scene and material related stuff
//     //  this is needed because the whole graphics pipeline is created here
//     //  but when actually allocating desc sets material should only allocate material desc sets
//     //  scene desc sets should be allocated outside (maybe in scene)
//     {
//         VkDescriptorSetLayoutBinding uniformBufferLayout{};
//         uniformBufferLayout.binding = 0;
//         uniformBufferLayout.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
//         uniformBufferLayout.descriptorCount = 1;
//         uniformBufferLayout.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
//         uniformBufferLayout.pImmutableSamplers = nullptr;
//         builder.addBinding(uniformBufferLayout);
//     }
//     {
//         VkDescriptorSetLayoutBinding uniformBufferLayout{};
//         uniformBufferLayout.binding = 1;
//         uniformBufferLayout.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
//         uniformBufferLayout.descriptorCount = 1;
//         uniformBufferLayout.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
//         uniformBufferLayout.pImmutableSamplers = nullptr;
//         builder.addBinding(uniformBufferLayout);
//     }

//     mDescriptorSetLayout = builder.Build(mDevice);
// }

std::unique_ptr<BasicBlinnPhongMaterial> BasicBlinnPhongMaterial::Builder::buildMaterial(VkDevice device) const
{
    assert(device != nullptr);

    std::unique_ptr<BasicBlinnPhongMaterial> material = std::make_unique<BasicBlinnPhongMaterial>(CARROT, device);

    material->mPipeline = buildPipeline(device);

    //  no material descriptor required for now
    //  init descriptor allocator
    // constexpr size_t MAX_SETS = 2; //   should be max frames inflight
    // DescriptorAllocator::PoolSize poolSizes[] = {
    //     {.mType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .mRatio = 2},
    // };
    // material->mDescriptorAllocator.Init(device, MAX_SETS, poolSizes);

    return std::move(material);
}

MaterialPipeline BasicBlinnPhongMaterial::Builder::buildPipeline(VkDevice device) const
{
    VkPipeline pipeline;
    VkPipelineLayout pipelineLayout;

    //  load shader file
    Shader vertexShader(VERTEX_SHADER_PATH, device);
    Shader fragmentShader(FRAGMENT_SHADER_PATH, device);

    //  build pipeline

    //  no material descriptor required for now
    //  build material descriptor set layout
    // buildDescriptorSetLayout(device);

    VkDescriptorSetLayout layouts[] = {mSceneDescSetLayout, mObjectDescSetLayout};

    //  pipeline layout
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 2;
    pipelineLayoutInfo.pSetLayouts = layouts;
    pipelineLayoutInfo.pushConstantRangeCount = mPushConstantRanges.size();
    pipelineLayoutInfo.pPushConstantRanges = mPushConstantRanges.empty() ? nullptr : mPushConstantRanges.data();

    VK_HARD_CHECK(vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout));

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
    builder.setColorAttachmentFormat(mColorFormat);
    builder.setDepthFormat(mDepthFormat);
    builder.setPipelineLayout(pipelineLayout);

    pipeline = builder.build(device);

    return MaterialPipeline{.mPipeline = pipeline, .mPipelineLayout = pipelineLayout};
}

} // namespace Bunny::Render
