#include "WorldLoader.h"

#include "MeshBank.h"
#include "MaterialBank.h"
#include "Transform.h"
#include "WorldComponents.h"
#include "WorldLoaderHelper.h"

#include <glm/mat4x4.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtx/euler_angles.hpp>

#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/core.hpp>
#include <fastgltf/tools.hpp>
#include <fastgltf/util.hpp>

#include <random>
#include <cassert>

namespace Bunny::Engine
{

WorldLoader::WorldLoader(const Render::VulkanRenderResources* vulkanResources, Render::PbrMaterialBank* pbrMaterialBank,
    Render::MeshBank<Render::NormalVertex>* meshBank)
    : mVulkanResources(vulkanResources),
      mPbrMaterialBank(pbrMaterialBank),
      mMeshBank(meshBank)
{
}

BunnyResult WorldLoader::loadPbrTestWorldWithGltfMeshes(std::string_view filePath, World& outWorld)
{
    assert(mPbrMaterialBank != nullptr);

    std::filesystem::path path(filePath);
    constexpr auto gltfOptions = fastgltf::Options::DontRequireValidAssetMember | fastgltf::Options::AllowDouble |
                                 fastgltf::Options::LoadGLBBuffers | fastgltf::Options::LoadExternalBuffers |
                                 fastgltf::Options::DecomposeNodeMatrices;
    // fastgltf::Options::LoadExternalImages;
    auto loadedFile = fastgltf::GltfDataBuffer::FromPath(path);
    if (loadedFile.error() != fastgltf::Error::None)
    {
        return BUNNY_SAD;
    }
    fastgltf::Parser parser;
    auto parseResult = parser.loadGltf(loadedFile.get(), path.parent_path(), gltfOptions);
    if (parseResult.error() != fastgltf::Error::None)
    {
        return BUNNY_SAD;
    }
    fastgltf::Asset& gltf = parseResult.get();

    //  load meshes
    loadMeshFromGltf(mMeshBank, mPbrMaterialBank, gltf);
    mMeshBank->buildMeshBuffers();

    static constexpr float spawnAreaXMax = 10;
    static constexpr float spawnAreaYMax = 10;
    static constexpr float spawnAreaZMax = 5;
    static constexpr uint32_t objectCount = 30;
    static constexpr float scaleMax = 2.0f;
    static constexpr float scaleMin = 0.8f;

    static constexpr float intervalX = 2;
    static constexpr float startX = -static_cast<float>(objectCount - 1) / 2 * intervalX;

    for (uint32_t i = 0; i < objectCount; i++)
    {
        float offsetX = startX + i * intervalX;
        Base::Transform nodeTransform({offsetX, 0, 0}, {0, 0, 0}, {1, 1, 1});

        const auto nodeEntity = outWorld.mEntityRegistry.create();
        outWorld.mEntityRegistry.emplace<TransformComponent>(nodeEntity, nodeTransform);
        //  assign a random material instance from the pbr material bank instead of using the one from the mesh
        outWorld.mEntityRegistry.emplace<MeshComponent>(
            nodeEntity, mMeshBank->getRandomMeshId(), mPbrMaterialBank->giveMeAMaterialInstance());
    }

    //  camera
    {
        const auto cameraEntity = outWorld.mEntityRegistry.create();
        Render::PhysicalCamera camera(glm::vec3{0, 5, 10}, glm::vec3{-glm::pi<float>() / 16, 0, 0});
        camera.setAperture(4);
        camera.setShutterTime(1.0f / 1600);
        outWorld.mEntityRegistry.emplace<PbrCameraComponent>(cameraEntity, camera);
    }

    //  light
    {
        const auto lightEntity = outWorld.mEntityRegistry.create();
        Render::PbrLight light{
            .mDirOrPos = glm::normalize(glm::vec3{-1,   -3,   -2  }
              ),
            .mIntensity = 100000, //  10AM sun light
            .mColor = {1.0f, 1.0f, 1.0f},
            .mType = Render::LightType::Directional,
        };
        outWorld.mEntityRegistry.emplace<PbrLightComponent>(lightEntity, light);
    }

    postLoad(outWorld);

    return BUNNY_HAPPY;
}

void Engine::WorldLoader::postLoad(World& outWorld)
{
    //  sort the meshes and transform component according to mesh id
    //  to prepare them for rendering
    outWorld.mEntityRegistry.sort<MeshComponent>(
        [](const MeshComponent& lhs, const MeshComponent& rhs) { return lhs.mMeshId < rhs.mMeshId; });
    outWorld.mEntityRegistry.sort<TransformComponent, MeshComponent>();
}

} // namespace Bunny::Engine
