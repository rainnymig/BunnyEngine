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
#include "ForwardPass.h"
#include "CullingPass.h"
#include "DepthReducePass.h"
#include "GBufferPass.h"
#include "DeferredShadingPass.h"

#include <imgui.h>
#include <fmt/core.h>
#include <inicpp.h>
#include <entt/entt.hpp>
#include <memory>

using namespace Bunny::Engine;

int main(void)
{
    fmt::print("Welcome to Bunny Engine!\n");

#ifdef _DEBUG
    fmt::print("This is DEBUG build.\n");
#else
    fmt::print("This is RELEASE build.\n");
#endif

    Config config;
    // config.loadConfigFile("./config.ini");

    Bunny::Base::Window window;
    window.initialize(config.mWindowWidth, config.mWindowHeight, config.mIsFullScreen, config.mWindowName);

    Bunny::Base::InputManager inputManager;
    inputManager.setupWithWindow(window);

    Bunny::Render::VulkanRenderResources renderResources;

    if (!BUNNY_SUCCESS(renderResources.initialize(&window)))
    {
        PRINT_AND_ABORT("Fail to initialize render resources.")
    }

    Bunny::Render::VulkanGraphicsRenderer renderer(&renderResources);

    if (!BUNNY_SUCCESS(renderer.initialize()))
    {
        PRINT_AND_ABORT("Fail to initialize graphics renderer.")
    }

    bool shouldRun = true;

    Bunny::Base::BasicTimer timer;

    Bunny::Render::MeshBank<Bunny::Render::NormalVertex> meshBank(&renderResources);
    Bunny::Render::MaterialBank materialBank;

    Bunny::Render::BasicBlinnPhongMaterial::Builder builder;
    builder.setColorAttachmentFormat(renderer.getSwapChainImageFormat());
    builder.setDepthFormat(renderer.getDepthImageFormat());
    std::unique_ptr<Bunny::Render::BasicBlinnPhongMaterial> blinnPhongMaterial =
        builder.buildMaterial(renderResources.getDevice());
    Bunny::Render::MaterialInstance blinnPhongInstance = blinnPhongMaterial->makeInstance();

    // Bunny::Render::ForwardPass forwardPass(&renderResources, &renderer, &materialBank, &meshBank);
    Bunny::Render::CullingPass cullingPass(&renderResources, &renderer, &meshBank);
    Bunny::Render::DepthReducePass depthReducePass(&renderResources, &renderer);
    Bunny::Render::GBufferPass gbufferPass(&renderResources, &renderer, &meshBank);
    Bunny::Render::DeferredShadingPass deferredShadingPass(&renderResources, &renderer);

    //  optimize later: detach these layouts from specific material
    // forwardPass.initializePass(blinnPhongMaterial->getSceneDescSetLayout(),
    //     blinnPhongMaterial->getObjectDescSetLayout(), blinnPhongMaterial->getDrawDescSetLayout());
    cullingPass.initializePass();
    depthReducePass.initializePass();
    gbufferPass.initializePass();
    deferredShadingPass.initializePass();

    materialBank.addMaterial(std::move(blinnPhongMaterial));
    materialBank.addMaterialInstance(blinnPhongInstance);

    World bunnyWorld;
    WorldLoader worldLoader(&renderResources, &materialBank, &meshBank);
    worldLoader.loadTestWorld(bunnyWorld);

    // forwardPass.buildDrawCommands();
    gbufferPass.buildDrawCommands();

    WorldRenderDataTranslator worldTranslator(&renderResources, &meshBank, &materialBank);
    worldTranslator.initialize();
    worldTranslator.initObjectDataBuffer(&bunnyWorld);

    // forwardPass.updateDrawInstanceCounts(worldTranslator.getMeshInstanceCounts());
    gbufferPass.updateDrawInstanceCounts(worldTranslator.getMeshInstanceCounts());

    // forwardPass.linkSceneData(worldTranslator.getSceneBuffer());
    // forwardPass.linkLightData(worldTranslator.getLightBuffer());
    // forwardPass.linkObjectData(worldTranslator.getObjectBuffer(), worldTranslator.getObjectBufferSize());
    gbufferPass.linkSceneData(worldTranslator.getSceneBuffer());
    gbufferPass.linkObjectData(worldTranslator.getObjectBuffer(), worldTranslator.getObjectBufferSize());
    deferredShadingPass.linkLightData(worldTranslator.getLightBuffer());
    deferredShadingPass.linkGBufferMaps(
        gbufferPass.getColorMaps(), gbufferPass.getFragPosMaps(), gbufferPass.getNormalTexCoordMaps());

    cullingPass.linkCullingData(depthReducePass.getDepthHierarchyImage(), depthReducePass.getDepthReduceSampler());
    cullingPass.linkMeshData(meshBank.getBoundsBuffer(), meshBank.getBoundsBufferSize());
    cullingPass.linkObjectData(worldTranslator.getObjectBuffer(), worldTranslator.getObjectBufferSize());
    // cullingPass.linkDrawData(forwardPass.getDrawCommandBuffer(), forwardPass.getDrawCommandBufferSize(),
    //     forwardPass.getInstanceObjectBuffer(), forwardPass.getInstanceObjectBufferSize());
    cullingPass.linkDrawData(gbufferPass.getDrawCommandBuffer(), gbufferPass.getDrawCommandBufferSize(),
        gbufferPass.getInstanceObjectBuffer(), gbufferPass.getInstanceObjectBufferSize());
    cullingPass.setObjectCount(worldTranslator.getObjectCount());
    cullingPass.setDepthImageSizes(depthReducePass.getDepthImageWidth(), depthReducePass.getDepthImageHeight(),
        depthReducePass.getDepthHierarchyLevels());

    CameraSystem cameraSystem(&inputManager);

    float accumulatedTime = 0;
    constexpr float interval = 0.5f;
    uint32_t accumulatedFrames = 0;
    float fps = 0;
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

        worldTranslator.updateSceneData(&bunnyWorld);

        const auto camComps = bunnyWorld.mEntityRegistry.view<CameraComponent>();
        if (!camComps.empty())
        {
            const auto& cam = bunnyWorld.mEntityRegistry.get<CameraComponent>(camComps.front());
            cullingPass.updateCullingData(cam.mCamera);
        }

        renderer.beginRenderFrame();

        // forwardPass.resetDrawCommands();
        gbufferPass.resetDrawCommands();

        cullingPass.dispatch();

        // forwardPass.draw();
        gbufferPass.draw();
        deferredShadingPass.draw();

        depthReducePass.dispatch();

        renderer.beginImguiFrame();

        ImGui::Begin("Game Stats");
        ImGui::Text(fmt::format("FPS: {}", fps).c_str());
        ImGui::Separator();
        ImGui::Text("W: forward S: backward A: left D: right");
        ImGui::Text("E: up C: down");
        ImGui::End();

        renderer.finishImguiFrame();

        renderer.finishRenderFrame();
        //
    }

    renderer.waitForRenderFinish();

    worldTranslator.cleanup();
    cullingPass.cleanup();
    // forwardPass.cleanup();
    depthReducePass.cleanup();
    deferredShadingPass.cleanup();
    gbufferPass.cleanup();

    meshBank.cleanup();
    materialBank.cleanup();

    renderer.cleanup();
    renderResources.cleanup();

    window.destroyAndTerminate();

    return 0;
}