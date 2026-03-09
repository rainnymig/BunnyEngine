#include "WorldLoader.h"

#include "MeshBank.h"
#include "MaterialBank.h"
#include "Transform.h"
#include "WorldComponents.h"
#include "WorldLoaderHelper.h"
#include "TextureBank.h"

#include <glm/mat4x4.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>

#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/tools.hpp>
#include <fastgltf/util.hpp>

#include <cassert>
#include <variant>
#include <unordered_map>

namespace Bunny::Engine
{

WorldLoader::WorldLoader(const Render::VulkanRenderResources* vulkanResources, Render::PbrMaterialBank* pbrMaterialBank,
    Render::MeshBank<Render::NormalVertex>* meshBank, Render::TextureBank* textureBank)
    : mVulkanResources(vulkanResources),
      mPbrMaterialBank(pbrMaterialBank),
      mMeshBank(meshBank),
      mTextureBank(textureBank)
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
    loadMeshFromGltf(mMeshBank, mPbrMaterialBank, mTextureBank, gltf);
    mMeshBank->buildMeshBuffers();

    //  load node transforms and scene structures
    loadWorldStructure(gltf, outWorld);

    //  if no camera defined in the scene, create one
    {
        const auto camComps = outWorld.mEntityRegistry.view<PbrCameraComponent>();
        if (camComps.empty())
        {
            const auto cameraEntity = outWorld.mEntityRegistry.create();
            Render::PhysicalCamera camera({0, 3, -10}, {0, 0, 0});
            camera.setAperture(4);
            camera.setShutterTime(1.0f / 2000);
            outWorld.mEntityRegistry.emplace<PbrCameraComponent>(cameraEntity, camera);
        }
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

void WorldLoader::loadWorldStructure(fastgltf::Asset& gltfAsset, World& outWorld)
{
    //  first iterate all nodes and create entity for them with transform
    size_t nodeCount = gltfAsset.nodes.size();
    //  build a map of node idx -> entity id
    std::unordered_map<size_t, entt::entity> nodeIdxToEntityMap;
    for (size_t idx = 0; idx < nodeCount; idx++)
    {
        const fastgltf::Node& gltfNode = gltfAsset.nodes[idx];

        const auto nodeEntity = outWorld.mEntityRegistry.create();
        Base::Transform transform;
        glm::vec3 camPos;
        glm::vec3 camEuler;
        std::visit(fastgltf::visitor{[&](fastgltf::math::fmat4x4 matrix) {
                                         glm::mat4 matrixGLM(matrix[0][0], matrix[0][1], matrix[0][2], matrix[0][3],
                                             matrix[1][0], matrix[1][1], matrix[1][2], matrix[1][3], matrix[2][0],
                                             matrix[2][1], matrix[2][2], matrix[2][3], matrix[3][0], matrix[3][1],
                                             matrix[3][2], matrix[3][3]);
                                         transform = Base::Transform(matrixGLM);
                                     },
                       [&](fastgltf::TRS trs) {
                           glm::vec3 tl(trs.translation[0], trs.translation[1], trs.translation[2]);
                           glm::quat rot(trs.rotation[3], trs.rotation[0], trs.rotation[1], trs.rotation[2]);
                           glm::vec3 sc(trs.scale[0], trs.scale[1], trs.scale[2]);

                           camPos = tl;
                           camEuler = glm::eulerAngles(rot);

                           transform = Base::Transform(tl, rot, sc);
                       }},
            gltfNode.transform);

        //  add the transform component
        outWorld.mEntityRegistry.emplace<TransformComponent>(nodeEntity, transform);

        //  if the node is a mesh or a camera, add mesh component and camera component
        if (gltfNode.meshIndex.has_value())
        {
            outWorld.mEntityRegistry.emplace<MeshComponent>(
                nodeEntity, static_cast<Render::IdType>(gltfNode.meshIndex.value()), 0u);
        }
        else if (gltfNode.cameraIndex.has_value())
        {
            Render::PhysicalCamera camera(camPos, camEuler);
            camera.setAperture(4);
            camera.setShutterTime(1.0f / 2000);
            outWorld.mEntityRegistry.emplace<PbrCameraComponent>(nodeEntity, camera);
        }

        //  save the node idx to entity mapping
        nodeIdxToEntityMap[idx] = nodeEntity;
    }

    //  iterate the nodes again to build the hierarchy
    for (size_t idx = 0; idx < nodeCount; idx++)
    {
        const fastgltf::Node& gltfNode = gltfAsset.nodes[idx];
        entt::entity thisNodeEntity = nodeIdxToEntityMap.at(idx);

        for (size_t childIdx : gltfNode.children)
        {
            entt::entity childNodeEntity = nodeIdxToEntityMap.at(childIdx);
            outWorld.mEntityRegistry.emplace<HierarchyComponent>(childNodeEntity, thisNodeEntity);
        }
    }
}

} // namespace Bunny::Engine
