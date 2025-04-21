#include "Material.h"

#include "Shader.h"
#include "Error.h"
#include "ErrorCheck.h"
#include "Vertex.h"
#include "GraphicsPipelineBuilder.h"

#include <cassert>

namespace Bunny::Render
{
Material::~Material()
{
}

BasicBlinnPhongMaterial::BasicBlinnPhongMaterial(Base::BunnyGuard<Builder> guard, VkDevice device) : mDevice(device)
{
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
    vkDestroyDescriptorSetLayout(mDevice, mSceneDescSetLayout, nullptr);
    vkDestroyDescriptorSetLayout(mDevice, mObjectDescSetLayout, nullptr);
    // vkDestroyDescriptorSetLayout(mDevice, mDescriptorSetLayout, nullptr);

    mDevice = nullptr;
}

MaterialInstance BasicBlinnPhongMaterial::makeInstance()
{
    //  Note: maybe create instance cache instead of recreating every time

    static int instanceIdCounter = 1;

    MaterialInstance newInstance;

    newInstance.mpBaseMaterial = &mPipeline;
    // mDescriptorAllocator.allocate(mDevice, &mDescriptorSetLayout, &newInstance.mDescriptorSet);

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

    if (!BUNNY_SUCCESS(
            buildDescriptorSetLayouts(device, material->mSceneDescSetLayout, material->mObjectDescSetLayout)))
    {
        PRINT_AND_RETURN_VALUE("Fail to build descriptor set layouts for Basic Blinn Phong Material", nullptr);
    }

    if (!BUNNY_SUCCESS(
            buildPipeline(device, material->mSceneDescSetLayout, material->mObjectDescSetLayout, material->mPipeline)))
    {
        PRINT_AND_RETURN_VALUE("Fail to build pipeline for Basic Blinn Phong Material", nullptr);
    }

    //  no material descriptor required for now
    //  init descriptor allocator
    // constexpr size_t MAX_SETS = 2; //   should be max frames inflight
    // DescriptorAllocator::PoolSize poolSizes[] = {
    //     {.mType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .mRatio = 2},
    // };
    // material->mDescriptorAllocator.Init(device, MAX_SETS, poolSizes);

    return std::move(material);
}

BunnyResult BasicBlinnPhongMaterial::Builder::buildPipeline(VkDevice device, VkDescriptorSetLayout sceneLayout,
    VkDescriptorSetLayout objectLayout, MaterialPipeline& outPipeline) const
{
    VkPipeline pipeline;
    VkPipelineLayout pipelineLayout;

    //  load shader file
    Shader vertexShader(mVertexShaderPath, device);
    Shader fragmentShader(mFragmentShaderPath, device);

    //  build pipeline

    VkDescriptorSetLayout layouts[] = {sceneLayout, objectLayout};

    //  pipeline layout
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 2;
    pipelineLayoutInfo.pSetLayouts = layouts;
    pipelineLayoutInfo.pushConstantRangeCount = 0;
    pipelineLayoutInfo.pPushConstantRanges = nullptr;

    VK_CHECK_OR_RETURN_BUNNY_SAD(vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout))

    //  vertex info
    auto bindingDescription = getBindingDescription<NormalVertex>(0, VertexInputRate::Vertex);
    auto attributeDescriptions = NormalVertex::getAttributeDescriptions();

    GraphicsPipelineBuilder builder;
    builder.setShaders(vertexShader.getShaderModule(), fragmentShader.getShaderModule());
    builder.setVertexInput(attributeDescriptions.data(), attributeDescriptions.size(), &bindingDescription, 1);
    builder.setInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    builder.setPolygonMode(VK_POLYGON_MODE_FILL);
    builder.setCulling(VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE);
    builder.setMultisamplingNone();
    builder.disableBlending(); //  opaque pipeline
    builder.enableDepthTest(VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL);
    // builder.enableDepthTest(VK_TRUE, VK_COMPARE_OP_GREATER_OR_EQUAL);
    builder.setColorAttachmentFormat(mColorFormat);
    builder.setDepthFormat(mDepthFormat);
    builder.setPipelineLayout(pipelineLayout);

    pipeline = builder.build(device);

    if (pipeline == nullptr)
    {
        PRINT_AND_RETURN_VALUE("Can not build Basic Blinn Phong Material pipeline", BUNNY_SAD)
    }

    outPipeline.mPipeline = pipeline;
    outPipeline.mPipelineLayout = pipelineLayout;

    return BUNNY_HAPPY;
}

BunnyResult BasicBlinnPhongMaterial::Builder::buildDescriptorSetLayouts(
    VkDevice device, VkDescriptorSetLayout& outSceneLayout, VkDescriptorSetLayout& outObjectLayout) const
{
    //  object data desc sets
    DescriptorLayoutBuilder layoutBuilder;
    {
        VkDescriptorSetLayoutBinding uniformBufferLayout{};
        uniformBufferLayout.binding = 0;
        uniformBufferLayout.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER; //  storage buffer?
        uniformBufferLayout.descriptorCount = 1;
        uniformBufferLayout.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        uniformBufferLayout.pImmutableSamplers = nullptr;
        layoutBuilder.addBinding(uniformBufferLayout);
    }
    outObjectLayout = layoutBuilder.build(device);

    //  scene data desc sets
    layoutBuilder.clear();
    {
        VkDescriptorSetLayoutBinding uniformBufferLayout{};
        uniformBufferLayout.binding = 0;
        uniformBufferLayout.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uniformBufferLayout.descriptorCount = 1;
        uniformBufferLayout.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        uniformBufferLayout.pImmutableSamplers = nullptr;
        layoutBuilder.addBinding(uniformBufferLayout);
        uniformBufferLayout.binding = 1;
        uniformBufferLayout.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        layoutBuilder.addBinding(uniformBufferLayout);
    }
    outSceneLayout = layoutBuilder.build(device);

    return BUNNY_HAPPY;
}

} // namespace Bunny::Render
