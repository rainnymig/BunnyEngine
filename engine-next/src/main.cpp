#include "Config.h"

#include "Window.h"
#include "Input.h"
#include "Timer.h"
#include "Error.h"
#include "VulkanRenderResources.h"
#include "VulkanGraphicsRenderer.h"
#include "World.h"
#include "WorldLoader.h"
#include "WorldDataTranslator.h"
#include "WorldComponents.h"
#include "WorldSystems.h"
#include "MeshBank.h"
#include "Material.h"
#include "MaterialBank.h"
#include "Vertex.h"
#include "CullingPass.h"
#include "DepthReducePass.h"
#include "GBufferPass.h"
#include "DeferredShadingPass.h"
#include "TextureBank.h"
#include "PbrForwardPass.h"
#include "AccelerationStructureBuilder.h"
#include "RaytracingShadowPass.h"
#include "ImguiHelper.h"

#include <imgui.h>
#include <fmt/core.h>
#include <inicpp.h>
#include <entt/entt.hpp>
#include <memory>

using namespace Bunny::Engine;
using namespace Bunny::Render;
using Bunny::Base::ImguiHelper;
using Bunny::Base::BasicTimer;

using PbrMaterialLoadParams = Bunny::Render::PbrMaterialBank::PbrMaterialLoadParams;

int main(void)
{
    fmt::print("Welcome to Bunny Engine!\n");

#ifdef _DEBUG
    fmt::print("This is DEBUG build.\n");
#else
    fmt::print("This is RELEASE build.\n");
#endif

    Config config;
    config.loadConfigFile("./assets/config.ini");

    Bunny::Base::Window window;
    window.initialize(config.mWindowWidth, config.mWindowHeight, config.mIsFullScreen, config.mWindowName);

    Bunny::Base::InputManager inputManager;
    inputManager.setupWithWindow(window);

    VulkanRenderResources renderResources;

    if (!BUNNY_SUCCESS(renderResources.initialize(&window)))
    {
        PRINT_AND_ABORT("Fail to initialize render resources.")
    }

    VulkanGraphicsRenderer renderer(&renderResources);

    if (!BUNNY_SUCCESS(renderer.initialize()))
    {
        PRINT_AND_ABORT("Fail to initialize graphics renderer.")
    }

    ImguiHelper::setup();

    BasicTimer timer;

    TextureBank textureBank(&renderResources, &renderer);
    MeshBank<NormalVertex> meshBank(&renderResources);
    PbrMaterialBank pbrMaterialBank(&renderResources, &renderer, &textureBank);

    textureBank.initialize();
    pbrMaterialBank.initialize();

    {
        IdType matInstId;
        pbrMaterialBank.addMaterialInstance(
            PbrMaterialLoadParams{
                .mBaseColor = glm::vec4(0.8f, 0.8f, 0.5f, 1.0f),
                .mEmissiveColor = glm::vec4(0, 0, 0, 0),
                .mMetallic = 0.05f,
                .mRoughness = 0.5f,
                .mReflectance = 0.5f,
            },
            matInstId);
        pbrMaterialBank.addMaterialInstance(
            PbrMaterialLoadParams{
                .mBaseColor = glm::vec4(0.5f, 0.8f, 0.9f, 1.0f),
                .mEmissiveColor = glm::vec4(0, 0, 0, 0),
                .mMetallic = 0.05f,
                .mRoughness = 0.5f,
                .mReflectance = 0.5f,
            },
            matInstId);
    }
    pbrMaterialBank.recreateMaterialBuffer();

    World bunnyWorld;
    WorldLoader worldLoader(&renderResources, &pbrMaterialBank, &meshBank);
    static constexpr std::string_view gltfFilePath = "./assets/model/both_smooth.glb";
    worldLoader.loadPbrTestWorldWithGltfMeshes(gltfFilePath, bunnyWorld);

    AccelerationStructureBuilder acceStructBuilder(&renderResources, &renderer);
    acceStructBuilder.buildBottomLevelAccelerationStructures(
        meshBank.getBlasGeometryData(), VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR);

    RaytracingShadowPass rtShadowPass(&renderResources, &renderer, &pbrMaterialBank, &meshBank);
    PbrForwardPass pbrForwardPass(&renderResources, &renderer, &pbrMaterialBank, &meshBank,
        "pbr_culled_instanced_vert.spv", "pbr_forward_frag.spv");
    CullingPass cullingPass(&renderResources, &renderer, &meshBank);
    DepthReducePass depthReducePass(&renderResources, &renderer);

    rtShadowPass.initializePass();
    pbrForwardPass.initializePass();
    cullingPass.initializePass();
    depthReducePass.initializePass();

    pbrForwardPass.buildDrawCommands();

    WorldRenderDataTranslator worldTranslator(&renderResources, &renderer, &meshBank);
    worldTranslator.initialize();
    worldTranslator.initObjectDataBuffer(&bunnyWorld);

    //  the top level acce struct needs to be updated when the objects move
    acceStructBuilder.buildTopLevelAccelerationStructures(
        worldTranslator.getObjectData(), VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR |
                                             VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR);

    rtShadowPass.updateVertIdxBufferData(meshBank.getVertexBufferAddress(), meshBank.getIndexBufferAddress());
    rtShadowPass.linkWorldData(worldTranslator.getPbrLightBuffer(), worldTranslator.getPbrCameraBuffer());
    rtShadowPass.linkObjectData(worldTranslator.getObjectBuffer(), worldTranslator.getObjectBufferSize());
    rtShadowPass.linkTopLevelAccelerationStructure(acceStructBuilder.getTopLevelAccelerationStructure().mAcceStruct);

    pbrForwardPass.updateDrawInstanceCounts(worldTranslator.getMeshInstanceCounts());
    pbrForwardPass.linkWorldData(worldTranslator.getPbrLightBuffer(), worldTranslator.getPbrCameraBuffer());
    pbrForwardPass.linkObjectData(worldTranslator.getObjectBuffer(), worldTranslator.getObjectBufferSize());
    pbrForwardPass.linkShadowData(rtShadowPass.getOutImageViews());

    cullingPass.linkCullingData(depthReducePass.getDepthHierarchyImage(), depthReducePass.getDepthReduceSampler());
    cullingPass.linkMeshData(meshBank.getBoundsBuffer(), meshBank.getBoundsBufferSize());
    cullingPass.linkObjectData(worldTranslator.getObjectBuffer(), worldTranslator.getObjectBufferSize());
    cullingPass.linkDrawData(pbrForwardPass.getDrawCommandBuffer(), pbrForwardPass.getDrawCommandBufferSize(),
        pbrForwardPass.getInstanceObjectBuffer(), pbrForwardPass.getInstanceObjectBufferSize());
    cullingPass.setObjectCount(worldTranslator.getObjectCount());
    cullingPass.setDepthImageSizes(depthReducePass.getDepthImageWidth(), depthReducePass.getDepthImageHeight(),
        depthReducePass.getDepthHierarchyLevels());

    CameraSystem cameraSystem(&inputManager);
    ObjectRandomMovementSystem objRandMovSystem;

    float accumulatedTime = 0;
    constexpr float interval = 0.5f;
    uint32_t accumulatedFrames = 0;
    float fps = 0;

    auto showBasicInfo = [&fps]() {
        ImGui::Begin("Game Stats");
        ImGui::Text(fmt::format("FPS: {}", fps).c_str());
        ImGui::Separator();
        ImGui::Text("W: forward S: backward A: left D: right");
        ImGui::Text("E: up C: down");
        ImGui::End();
    };
    ImguiHelper::get().registerCommand(showBasicInfo);
    auto showCamAndLightControl = [&bunnyWorld, &worldTranslator]() {
        worldTranslator.showImguiControlPanel(&bunnyWorld);
    };
    ImguiHelper::get().registerCommand(showCamAndLightControl);

    timer.start();
    while (true)
    {
        timer.tick();

        accumulatedTime += timer.getDeltaTime();
        accumulatedFrames++;
        if (accumulatedTime > interval)
        {
            fps = accumulatedFrames / accumulatedTime;
            accumulatedFrames = 0;
            accumulatedTime = 0;
        }

        if (window.processWindowEvent())
        {
            break;
        }

        cameraSystem.update(&bunnyWorld, timer.getDeltaTime());
        //  update object transforms
        objRandMovSystem.update(&bunnyWorld, timer.getDeltaTime(), timer.getTime());

        //  update object data buffer
        worldTranslator.updateObjectData(&bunnyWorld);

        //  update acceleration structures
        acceStructBuilder.buildTopLevelAccelerationStructures(worldTranslator.getObjectData(),
            VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR |
                VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR,
            true);

        worldTranslator.updatePbrWorldData(&bunnyWorld);
        pbrMaterialBank.updateMaterialBuffer();
        const auto camComps = bunnyWorld.mEntityRegistry.view<PbrCameraComponent>();
        if (!camComps.empty())
        {
            const auto& cam = bunnyWorld.mEntityRegistry.get<PbrCameraComponent>(camComps.front());
            cullingPass.updateCullingData(cam.mCamera);
        }

        renderer.beginRenderFrame();

        pbrForwardPass.prepareDrawCommandsForFrame();

        cullingPass.dispatch();
        rtShadowPass.draw();
        pbrForwardPass.draw();
        depthReducePass.dispatch();

        renderer.beginImguiFrame();
        ImguiHelper::get().render();
        renderer.finishImguiFrame();

        renderer.finishRenderFrame();
    }

    renderer.waitForRenderFinish();

    cullingPass.cleanup();
    depthReducePass.cleanup();
    pbrForwardPass.cleanup();
    rtShadowPass.cleanup();
    acceStructBuilder.cleanup();
    worldTranslator.cleanup();

    meshBank.cleanup();
    pbrMaterialBank.cleanup();
    textureBank.cleanup();

    renderer.cleanup();
    renderResources.cleanup();

    window.destroyAndTerminate();

    return 0;
}