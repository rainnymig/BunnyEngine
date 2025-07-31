#pragma once

#include "Fundamentals.h"
#include "VulkanRenderResources.h"
#include "BoundingBox.h"
#include "AccelerationStructureData.h"

#include <volk.h>

#include <vector>
#include <string>
#include <string_view>
#include <span>
#include <unordered_map>
#include <algorithm>
#include <iterator>
#include <random>

namespace Bunny::Render
{

struct SurfaceLite
{
    uint32_t mFirstIndex; //  the idx of the first index in the shared index buffer
    uint32_t mIndexCount; //  the number of indices
    IdType mMaterialId;
    IdType mMaterialInstanceId;
};

template <typename BoundType>
struct MeshLiteT
{
    IdType mId;
    std::string mName;
    std::vector<SurfaceLite> mSurfaces;
    BoundType mBounds;
    uint32_t mVertexOffset = 0;
};

using MeshLite = MeshLiteT<Base::BoundingSphere>;

template <typename VertexType, typename IndexType = uint32_t>
class MeshBank
{
  public:
    MeshBank(const VulkanRenderResources* vulkanResources) : mVulkanResources(vulkanResources) {}
    ~MeshBank();

    //  the indices should be unprocessed, i.e. as they are starting from 0
    IdType addMesh(std::span<VertexType> vertices, std::span<IndexType> indices, const MeshLite& mesh);
    void buildMeshBuffers();
    void bindMeshBuffers(VkCommandBuffer cmdBuf) const;
    const MeshLite& getMesh(IdType id) const { return mMeshes.at(id); }
    const MeshLite& getMesh(std::string_view name) const { return mMeshes.at(getMeshIdFromName(name)); }
    const std::vector<MeshLite>& getMeshes() const { return mMeshes; }
    const IdType getMeshIdFromName(std::string_view name) const { return mMeshNameToIdMap.at(name); }
    const IdType getRandomMeshId() const;
    void cleanup();

    const AllocatedBuffer& getBoundsBuffer() const { return mBoundsBuffer; }
    const size_t getBoundsBufferSize() const { return sizeof(Base::BoundingSphere) * mBoundsData.size(); }

    [[nodiscard]] std::vector<AcceStructGeometryData> getBlasGeometryData() const;

    VkDeviceAddress getVertexBufferAddress() const { return mVertexBufferAddress; }
    VkDeviceAddress getIndexBufferAddress() const { return mIndexBufferAddress; }

  private:
    AcceStructGeometryData buildTriangleBlasGeometryDataFromMesh(
        const MeshLite& mesh, VkDeviceAddress vertexBufferAddress, VkDeviceAddress indexBufferAdress) const;

    AllocatedBuffer mVertexBuffer;
    AllocatedBuffer mIndexBuffer;
    AllocatedBuffer mBoundsBuffer;

    VkDeviceAddress mVertexBufferAddress;
    VkDeviceAddress mIndexBufferAddress;

    std::vector<VertexType> mVertexBufferData;
    std::vector<IndexType> mIndexBufferData;
    std::vector<Base::BoundingSphere> mBoundsData; //  maybe template this as well

    std::vector<MeshLite> mMeshes;
    std::unordered_map<std::string_view, IdType> mMeshNameToIdMap;

    const VulkanRenderResources* mVulkanResources;
};

template <typename VertexType, typename IndexType>
MeshBank<VertexType, IndexType>::~MeshBank()
{
    cleanup();
}

template <typename VertexType, typename IndexType>
IdType MeshBank<VertexType, IndexType>::addMesh(
    std::span<VertexType> vertices, std::span<IndexType> indices, const MeshLite& mesh)
{
    //  take the current number of vertices in the vertex buffer
    //  because the newly added indices will have to count from here
    uint32_t vertexOffset = mVertexBufferData.size();
    //  take the current number of indices in the index buffer
    //  because the newly added surfaces will need to offset their starting index from here
    uint32_t indexOffset = mIndexBufferData.size();
    //  vertices can be directly insert into the buffer
    mVertexBufferData.insert(mVertexBufferData.end(), vertices.begin(), vertices.end());
    //  indices need to add the offset first
    //  update: no need, vertex offset set in mesh instead
    // std::transform(indices.begin(), indices.end(), std::back_inserter(mIndexBufferData),
    //     [vertexOffset](IndexType index) { return index + vertexOffset; });
    mIndexBufferData.insert(mIndexBufferData.end(), indices.begin(), indices.end());

    //  assign an id for the new mesh
    //  the id is it's index in the mMeshes vector
    //  this makes it easier to find the mesh using the id
    const IdType meshId = mMeshes.size();
    mMeshes.push_back(mesh);
    mMeshes[meshId].mId = meshId;
    mMeshes[meshId].mVertexOffset = vertexOffset;
    for (SurfaceLite& surface : mMeshes[meshId].mSurfaces)
    {
        surface.mFirstIndex += indexOffset;
    }
    mBoundsData.push_back(mMeshes[meshId].mBounds);

    //  update mesh name to id mapping to enable get mesh from name
    mMeshNameToIdMap[mMeshes[meshId].mName] = meshId;

    return meshId;
}

template <typename VertexType, typename IndexType>
void MeshBank<VertexType, IndexType>::buildMeshBuffers()
{
    const VkDeviceSize vertexSize = mVertexBufferData.size() * sizeof(VertexType);
    const VkDeviceSize indexSize = mIndexBufferData.size() * sizeof(IndexType);
    const VkDeviceSize boundsSize = mBoundsData.size() * sizeof(Base::BoundingSphere);

    //  add VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT for vertex and index buffer
    //  this is required when creating acceleration structures for ray tracing
    mVulkanResources->createBufferWithData(mVertexBufferData.data(), vertexSize,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
            VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        VMA_ALLOCATION_CREATE_MAPPED_BIT, VMA_MEMORY_USAGE_GPU_ONLY, mVertexBuffer);
    mVulkanResources->createBufferWithData(mIndexBufferData.data(), indexSize,
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
            VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        VMA_ALLOCATION_CREATE_MAPPED_BIT, VMA_MEMORY_USAGE_GPU_ONLY, mIndexBuffer);
    mVulkanResources->createBufferWithData(mBoundsData.data(), boundsSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        VMA_ALLOCATION_CREATE_MAPPED_BIT, VMA_MEMORY_USAGE_GPU_ONLY, mBoundsBuffer); //  bounds of meshes for culling

    mVertexBufferAddress = mVulkanResources->getBufferDeviceAddress(mVertexBuffer);
    mIndexBufferAddress = mVulkanResources->getBufferDeviceAddress(mIndexBuffer);
}

template <typename VertexType, typename IndexType>
void MeshBank<VertexType, IndexType>::bindMeshBuffers(VkCommandBuffer cmdBuf) const
{
    VkBuffer vertexBuffers[] = {mVertexBuffer.mBuffer};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(cmdBuf, 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(cmdBuf, mIndexBuffer.mBuffer, 0, VK_INDEX_TYPE_UINT32);
    //  bounds buffer shall be bound in cull (compute) pass
}

template <typename VertexType, typename IndexType>
inline const IdType MeshBank<VertexType, IndexType>::getRandomMeshId() const
{
    static std::random_device rd;
    static std::mt19937 re(rd());

    std::uniform_int_distribution<int> uniDist(0, mMeshes.size() - 1);
    return uniDist(re);
}

template <typename VertexType, typename IndexType>
void MeshBank<VertexType, IndexType>::cleanup()
{
    mVulkanResources->destroyBuffer(mVertexBuffer);
    mVulkanResources->destroyBuffer(mIndexBuffer);
    mVulkanResources->destroyBuffer(mBoundsBuffer);

    mVertexBufferData.clear();
    mIndexBufferData.clear();
    mBoundsData.clear();
}

template <typename VertexType, typename IndexType>
inline std::vector<AcceStructGeometryData> MeshBank<VertexType, IndexType>::getBlasGeometryData() const
{
    std::vector<AcceStructGeometryData> blasGeometryData;

    for (const MeshLite& mesh : mMeshes)
    {
        blasGeometryData.emplace_back(
            buildTriangleBlasGeometryDataFromMesh(mesh, mVertexBufferAddress, mIndexBufferAddress));
    }

    return blasGeometryData;
}

template <typename VertexType, typename IndexType>
inline AcceStructGeometryData MeshBank<VertexType, IndexType>::buildTriangleBlasGeometryDataFromMesh(
    const MeshLite& mesh, VkDeviceAddress vertexBufferAddress, VkDeviceAddress indexBufferAdress) const
{
    AcceStructGeometryData blasData;

    static constexpr uint32_t indexCountPerTriangle = 3;

    for (const SurfaceLite& surface : mesh.mSurfaces)
    {
        VkAccelerationStructureGeometryTrianglesDataKHR triangles{
            VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR};

        triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT; // vec3 vertex position data.

        triangles.vertexData.deviceAddress = vertexBufferAddress;
        triangles.vertexStride = sizeof(VertexType);
        triangles.indexType = VK_INDEX_TYPE_UINT32; // hardcoded here, update later to make it change with IndexType
        triangles.indexData.deviceAddress = indexBufferAdress;
        //  Indicate identity transform by setting transformData to null device pointer.
        //  triangles.transformData = {};
        triangles.maxVertex = mVertexBufferData.size() - 1;

        VkAccelerationStructureGeometryKHR& geometry =
            blasData.mGeometries.emplace_back(VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR);
        geometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
        geometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR; //  for now everything is opaque
        geometry.geometry.triangles = triangles;

        VkAccelerationStructureBuildRangeInfoKHR& offset = blasData.mBuildRanges.emplace_back();
        offset.firstVertex = mesh.mVertexOffset;
        offset.primitiveCount = surface.mIndexCount / indexCountPerTriangle;
        offset.primitiveOffset = 0;
        offset.transformOffset = 0;
    }

    return blasData;
}
} // namespace Bunny::Render
