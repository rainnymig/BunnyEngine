#include "World.h"

#include "VulkanRenderResources.h"
#include "Vertex.h"
#include "MaterialBank.h"
#include "Helper.h"

#include <glm/mat4x4.hpp>
#include <glm/ext/matrix_transform.hpp>

#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/core.hpp>
#include <fastgltf/tools.hpp>
#include <fastgltf/util.hpp>

#include <filesystem>
#include <algorithm>

namespace Bunny::Engine
{
BunnyResult WorldLoader::loadGltfToWorld(
    std::string_view filePath, World& outWorld)
{
    std::filesystem::path path(filePath);
    constexpr auto gltfOptions = fastgltf::Options::DontRequireValidAssetMember | fastgltf::Options::AllowDouble |
                                 fastgltf::Options::LoadGLBBuffers | fastgltf::Options::LoadExternalBuffers;
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
    std::vector<uint32_t> indices;
    std::vector<Render::NormalVertex> vertices;
    for (fastgltf::Mesh& mesh : gltf.meshes)
    {
        Render::MeshLite newMesh;
        newMesh.mName = mesh.name;
        newMesh.mId = std::hash<std::string>{}(newMesh.mName);

        indices.clear();
        vertices.clear();

        glm::vec3 maxCorner {-100000, -100000, -100000};
        glm::vec3 minCorner {100000, 100000, 100000};

        //  create mesh surfaces
        for (auto&& primitive : mesh.primitives)
        {
            Render::SurfaceLite newSurface;
            newSurface.mFirstIndex = indices.size();
            size_t initialVtx = vertices.size();

            //  load indices
            {
                fastgltf::Accessor& indexAccessor = gltf.accessors[primitive.indicesAccessor.value()];
                newSurface.mIndexCount = indexAccessor.count;
                indices.reserve(indices.size() + indexAccessor.count);
                fastgltf::iterateAccessor<uint32_t>(gltf, indexAccessor, [&](uint32_t idx) { indices.push_back(idx); });
            }

            //  load vertex positions
            //  calculate bounding sphere in the process
            {
                fastgltf::Accessor& posAccessor = gltf.accessors[primitive.findAttribute("POSITION")->accessorIndex];
                vertices.resize(vertices.size() + posAccessor.count);

                fastgltf::iterateAccessorWithIndex<glm::vec3>(
                    gltf, posAccessor, [&vertices, &minCorner, &maxCorner, initialVtx](glm::vec3 vec, size_t idx) {
                        Render::NormalVertex newVertex;
                        newVertex.mPosition = vec;
                        newVertex.mNormal = {1, 0, 0};
                        newVertex.mColor = {0.8f, 0.8f, 0.8f, 1.0f};
                        newVertex.mTexCoord = {0, 0};
                        vertices[initialVtx + idx] = newVertex;

                        minCorner.x = std::min(minCorner.x, vec.x);
                        minCorner.y = std::min(minCorner.y, vec.y);
                        minCorner.z = std::min(minCorner.z, vec.z);
                        maxCorner.x = std::max(maxCorner.x, vec.x);
                        maxCorner.y = std::max(maxCorner.y, vec.y);
                        maxCorner.z = std::max(maxCorner.z, vec.z);
                    });
            }

            //  load vertex normal
            auto normalAttr = primitive.findAttribute("NORMAL");
            if (normalAttr != primitive.attributes.end())
            {
                fastgltf::Accessor& normalAccessor = gltf.accessors[normalAttr->accessorIndex];
                fastgltf::iterateAccessorWithIndex<glm::vec3>(gltf, normalAccessor,
                    [&vertices, initialVtx](glm::vec3 norm, size_t idx) { vertices[initialVtx + idx].mNormal = norm; });
            }

            //  load vertex color
            auto colorAttr = primitive.findAttribute("COLOR_0");
            if (colorAttr != primitive.attributes.end())
            {
                fastgltf::Accessor& colorAccessor = gltf.accessors[colorAttr->accessorIndex];
                fastgltf::iterateAccessorWithIndex<glm::vec4>(gltf, colorAccessor,
                    [&vertices, initialVtx](glm::vec4 col, size_t idx) { vertices[initialVtx + idx].mColor = col; });
            }

            //  load vertex texcoord
            auto TexCoordAttr = primitive.findAttribute("TEXCOORD_0");
            if (TexCoordAttr != primitive.attributes.end())
            {
                fastgltf::Accessor& texCoordAccessor = gltf.accessors[TexCoordAttr->accessorIndex];
                fastgltf::iterateAccessorWithIndex<glm::vec2>(
                    gltf, texCoordAccessor, [&vertices, initialVtx](glm::vec2 coord, size_t idx) {
                        vertices[initialVtx + idx].mTexCoord = coord;
                    });
            }

            //  load material
            //  for now all use default material
            newSurface.mMaterialInstanceId = mMaterialBank->getDefaultMaterialId();

            newMesh.mSurfaces.push_back(newSurface);
        }

        //  calculate bounding sphere of mesh
        newMesh.mBounds.mCenter = (minCorner + maxCorner) / 2.0f;
        newMesh.mBounds.mRadius = glm::length(maxCorner-minCorner) / 2.0f;

        //  create mesh buffers
        mMeshBank->addMesh(vertices, indices, newMesh);
    }

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

                                         glm::mat4 tm = glm::translate(glm::mat4(1.f), tl);
                                         glm::mat4 rm = glm::toMat4(rot);
                                         glm::mat4 sm = glm::scale(glm::mat4(1.f), sc);

                                         nodeTransform.mMatrix = tm * rm * sm;
                                     },
                       [&nodeTransform](fastgltf::math::fmat4x4 transformMat) {
                           memcpy(&nodeTransform.mMatrix, transformMat.data(), sizeof(transformMat));
                       }},
            node.transform);
        outWorld.mEntityRegistry.emplace<TransformComponent>(nodeEntity, nodeTransform);

        //  load node mesh if present
        if (node.meshIndex.has_value())
        {
            std::string meshName {gltf.meshes.at(node.meshIndex.value()).name.c_str()};
            size_t meshId = std::hash<std::string>{}(meshName);
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
            HierarchyComponent& childHierarchyComp = outWorld.mEntityRegistry.emplace_or_replace<HierarchyComponent>(nodeIdxToEntity.at(c));
            childHierarchyComp.mParent = nodeEntity;
        }
    }

    return BUNNY_HAPPY;
}

BunnyResult WorldLoader::loadTestWorld(World& outWorld)
{
    //  vecters to hold indices and vertices for creating buffers
    // std::vector<uint32_t> indices;
    // std::vector<NormalVertex> vertices;
    // std::unordered_map<NormalVertex, uint32_t, NormalVertex::Hash> vertexToIndexMap;

    //  create meshes and save them in the mesh asset bank
    const Render::IdType meshId = createCubeMeshToBank(mMeshBank, mMaterialBank->getDefaultMaterialId());

    //  create scene structure
    //  create grid of cubes to fill the scene

    constexpr int resolution = 2;
    constexpr float gap = 2;
    // glm::vec3 pos{-gap * resolution / 2, -gap * resolution / 2, -gap * resolution / 2};
    // scene->mNodes.clear();
    // size_t idx = 0;
    // for (int z = 0; z < resolution; z++)
    // {
    //     for (int y = 0; y < resolution; y++)
    //     {
    //         for (int x = 0; x < resolution; x++)
    //         {
    //             Node& newNode = scene->mNodes[idx++];
    //             newNode.mScene = scene;
    //             newNode.mTransform = Base::Transform{
    //                 pos + glm::vec3{x * gap, y * gap, z * gap},
    //                   {0,       0,       0      },
    //                   {1,       1,       1      }
    //             };
    //             newNode.mRenderComponent = std::make_unique<MeshRenderComponent>(cubeMesh, &newNode);
    //         }
    //     }
    // }
    std::vector<glm::vec3> positions{
        {0, 0, 0},
        {2, 0, 0},
        {4, 0, 0},
        {0, 2, 0},
        {0, 4, 0},
        {0, 6, 0},
        {0, 0, 2},
        {0, 0, 4},
        {0, 0, 6},
        {0, 0, 8}
    };

    //  nodes
    for (int idx = 0; idx < positions.size(); idx++)
    {
        Base::Transform nodeTransform (positions[idx], {0, 0, 0}, {1, 1, 1});
        const auto nodeEntity = outWorld.mEntityRegistry.create();
        outWorld.mEntityRegistry.emplace<TransformComponent>(nodeEntity, nodeTransform);
        outWorld.mEntityRegistry.emplace<MeshComponent>(nodeEntity, meshId);
    }

    //  camera
    {
        const auto cameraEntity = outWorld.mEntityRegistry.create();
        Render::Camera camera(glm::vec3{5, 5, -5});
        outWorld.mEntityRegistry.emplace<CameraComponent>(cameraEntity, camera);
    }

    //  light
    {
        const auto lightEntity = outWorld.mEntityRegistry.create();
        Render::DirectionalLight dirLight{
            .mDirection = glm::normalize(glm::vec3{-1, -1, 1}
            ),
            .mColor = {1,  1,  1},
        };
        outWorld.mEntityRegistry.emplace<DirectionLightComponent>(lightEntity, dirLight);
    }

    return BUNNY_HAPPY;
}

} // namespace Bunny::Engine
