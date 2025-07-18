#include "WorldLoader.h"

#include "Helper.h"
#include "MeshBank.h"
#include "MaterialBank.h"
#include "Transform.h"
#include "WorldComponents.h"

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
WorldLoader::WorldLoader(const Render::VulkanRenderResources* vulkanResources, Render::MaterialBank* materialBank,
    Render::MeshBank<Render::NormalVertex>* meshBank)
    : mVulkanResources(vulkanResources),
      mMaterialBank(materialBank),
      mPbrMaterialBank(nullptr),
      mMeshBank(meshBank)
{
}

WorldLoader::WorldLoader(const Render::VulkanRenderResources* vulkanResources, Render::PbrMaterialBank* pbrMaterialBank,
    Render::MeshBank<Render::NormalVertex>* meshBank)
    : mVulkanResources(vulkanResources),
      mMaterialBank(nullptr),
      mPbrMaterialBank(pbrMaterialBank),
      mMeshBank(meshBank)
{
}

BunnyResult WorldLoader::loadGltfToWorld(std::string_view filePath, World& outWorld)
{
    assert(mMaterialBank != nullptr);

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

    //  load materials

    //  load textures

    //  load meshes
    Render::loadMeshFromGltf(mMeshBank, mMaterialBank, gltf);

    //  save the mapping from fastgltf node idx to entity
    //  to convert the children
    std::vector<entt::entity> nodeIdxToEntity;
    nodeIdxToEntity.reserve(gltf.nodes.size());

    //  load world structure
    //  load game object local transforms
    for (const fastgltf::Node& node : gltf.nodes)
    {

        const auto nodeEntity = outWorld.mEntityRegistry.create();
        nodeIdxToEntity.push_back(nodeEntity);
        Base::Transform nodeTransform;

        //  load node (local) transform
        std::visit(fastgltf::visitor{[&nodeTransform](fastgltf::TRS trs) {
                                         glm::vec3 tl(trs.translation[0], trs.translation[1], trs.translation[2]);
                                         glm::quat rot(
                                             trs.rotation[3], trs.rotation[0], trs.rotation[1], trs.rotation[2]);
                                         glm::vec3 sc(trs.scale[0], trs.scale[1], trs.scale[2]);

                                         //  glm::mat4 tm = glm::translate(glm::mat4(1.f), tl);
                                         //  glm::mat4 rm = glm::toMat4(rot);
                                         //  glm::mat4 sm = glm::scale(glm::mat4(1.f), sc);

                                         //  nodeTransform.mMatrix = tm * rm * sm;

                                         nodeTransform = Base::Transform(tl, rot, sc);
                                     },
                       [&nodeTransform](fastgltf::math::fmat4x4 transformMat) {
                           //  should not happen with fastgltf::Options::DecomposeNodeMatrices
                           //    memcpy(&nodeTransform.mMatrix, transformMat.data(), sizeof(transformMat));
                       }},
            node.transform);

        outWorld.mEntityRegistry.emplace<TransformComponent>(nodeEntity, nodeTransform);

        //  load node mesh if present
        if (node.meshIndex.has_value())
        {
            std::string meshName{gltf.meshes.at(node.meshIndex.value()).name.c_str()};
            Render::IdType meshId = mMeshBank->getMeshIdFromName(meshName);
            outWorld.mEntityRegistry.emplace<MeshComponent>(nodeEntity, meshId);
        }
    }

    //  load node scene structure
    for (int i = 0; i < gltf.nodes.size(); i++)
    {
        fastgltf::Node& node = gltf.nodes[i];
        const auto nodeEntity = nodeIdxToEntity.at(i);

        for (auto c : node.children)
        {
            HierarchyComponent& childHierarchyComp =
                outWorld.mEntityRegistry.emplace_or_replace<HierarchyComponent>(nodeIdxToEntity.at(c));
            childHierarchyComp.mParent = nodeEntity;
        }
    }

    postLoad(outWorld);

    return BUNNY_HAPPY;
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
    Render::loadMeshFromGltf(mMeshBank, mPbrMaterialBank, gltf);
    mMeshBank->buildMeshBuffers();

    static constexpr float spawnAreaXMax = 10;
    static constexpr float spawnAreaYMax = 10;
    static constexpr float spawnAreaZMax = 5;
    static constexpr uint32_t objectCount = 30;
    static constexpr float scaleMax = 2.0f;
    static constexpr float scaleMin = 0.8f;

    std::random_device rd;
    std::mt19937 re(rd());
    std::uniform_real_distribution<float> xDist(-spawnAreaXMax, spawnAreaXMax);
    std::uniform_real_distribution<float> yDist(-spawnAreaYMax, spawnAreaYMax);
    std::uniform_real_distribution<float> zDist(-spawnAreaZMax, spawnAreaZMax);
    std::uniform_real_distribution<float> scaleDist(scaleMin, scaleMax);
    std::uniform_real_distribution<float> rotDist(-glm::pi<float>(), glm::pi<float>());

    for (uint32_t i = 0; i < objectCount; i++)
    {
        float nodeScale = scaleDist(re);
        Base::Transform nodeTransform({xDist(re), yDist(re), zDist(re)}, {rotDist(re), rotDist(re), rotDist(re)},
            {nodeScale, nodeScale, nodeScale});
        const auto nodeEntity = outWorld.mEntityRegistry.create();
        outWorld.mEntityRegistry.emplace<TransformComponent>(nodeEntity, nodeTransform);
        //  assign a random material instance from the pbr material bank instead of using the one from the mesh
        outWorld.mEntityRegistry.emplace<MeshComponent>(
            nodeEntity, mMeshBank->getRandomMeshId(), mPbrMaterialBank->giveMeAMaterialInstance());
    }

    //  camera
    {
        const auto cameraEntity = outWorld.mEntityRegistry.create();
        Render::PhysicalCamera camera(glm::vec3{0, 5, 15}, glm::vec3{-glm::pi<float>() / 8, 0, 0});
        camera.setAperture(8);
        camera.setShutterTime(1.0f / 2000);
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

BunnyResult WorldLoader::loadTestWorld(World& outWorld)
{
    assert(mMaterialBank != nullptr);

    //  create meshes and save them in the mesh asset bank
    const Render::IdType meshId = Render::createCubeMeshToBank(
        mMeshBank, mMaterialBank->getDefaultMaterialId(), mMaterialBank->getDefaultMaterialInstanceId());
    mMeshBank->buildMeshBuffers();

    //  create scene structure
    //  create grid of cubes to fill the scene
    constexpr int resolution = 100;
    constexpr float gap = 2;
    std::vector<glm::vec3> positions;
    glm::vec3 startingPos{-gap * resolution / 2, -gap * resolution / 2, -gap * resolution / 2};
    size_t idx = 0;
    for (int z = 0; z < resolution; z++)
    {
        for (int y = 0; y < resolution; y++)
        {
            for (int x = 0; x < resolution; x++)
            {
                positions.emplace_back(
                    glm::vec3{startingPos.x + x * gap, startingPos.y + y * gap, startingPos.z + z * gap});
            }
        }
    }
    // std::vector<glm::vec3> positions{
    //     {0, 0, 0},
    //     {2, 0, 0},
    //     {4, 0, 0},
    //     {0, 2, 0},
    //     {0, 4, 0},
    //     {0, 6, 0},
    //     {0, 0, 2},
    //     {0, 0, 4},
    //     {0, 0, 6},
    //     {0, 0, 8}
    // };

    //  nodes
    for (int idx = 0; idx < positions.size(); idx++)
    {
        Base::Transform nodeTransform(positions[idx], {0, 0, 0}, {1, 1, 1});
        const auto nodeEntity = outWorld.mEntityRegistry.create();
        outWorld.mEntityRegistry.emplace<TransformComponent>(nodeEntity, nodeTransform);
        outWorld.mEntityRegistry.emplace<MeshComponent>(
            nodeEntity, meshId, mMaterialBank->getDefaultMaterialInstanceId());
    }

    //  camera
    {
        const auto cameraEntity = outWorld.mEntityRegistry.create();
        Render::Camera camera(glm::vec3{0, 5, 15}, glm::vec3{-glm::pi<float>() / 8, 0, 0});
        outWorld.mEntityRegistry.emplace<CameraComponent>(cameraEntity, camera);
    }

    //  light
    {
        const auto lightEntity = outWorld.mEntityRegistry.create();
        Render::DirectionalLight dirLight{
            .mDirection = glm::normalize(glm::vec3{-1, -3, -2}
              ),
            .mColor = {1,  1,  1 },
        };
        outWorld.mEntityRegistry.emplace<DirectionLightComponent>(lightEntity, dirLight);
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
