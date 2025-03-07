#pragma once

#include "Fundamentals.h"
#include "VulkanRenderResources.h"
#include "BoundingBox.h"

#include <vulkan/vulkan.h>

#include <vector>
#include <string>
#include <span>
#include <unordered_map>
#include <algorithm>
#include <iterator>

namespace Bunny::Render
{

struct SurfaceLite
{
    uint32_t mFirstIndex; //  the idx of the first index in the shared index buffer
    uint32_t mIndexCount; //  the number of indices
    IdType mMaterialInstanceId;
};

template<typename BoundType>
struct MeshLiteT
{
    IdType mId;
    std::string mName;
    std::vector<SurfaceLite> mSurfaces;
    BoundType mBounds;
};

using MeshLite = MeshLiteT<Base::BoundingSphere>;

template <typename VertexType, typename IndexType = uint32_t>
class MeshBank
{
  public:
    MeshBank(const VulkanRenderResources* vulkanResources) : mVulkanResources(vulkanResources) {}
    ~MeshBank();

    //  the indices should be unprocessed, i.e. as they are starting from 0
    void addMesh(std::span<VertexType> vertices, std::span<IndexType> indices, const MeshLite& mesh);
    void buildMeshBuffers();
    const MeshLite& getMesh(IdType id) const { return mMeshes.at(id); }
    void cleanup();

  private:
    AllocatedBuffer mVertexBuffer;
    AllocatedBuffer mIndexBuffer;
    std::vector<VertexType> mVertexBufferData;
    std::vector<IndexType> mIndexBufferData;
    std::unordered_map<IdType, MeshLite> mMeshes;
    const VulkanRenderResources* mVulkanResources;
};

template <typename VertexType, typename IndexType>
inline MeshBank<VertexType, IndexType>::~MeshBank()
{
    cleanup();
}

template <typename VertexType, typename IndexType>
inline void MeshBank<VertexType, IndexType>::addMesh(
    std::span<VertexType> vertices, std::span<IndexType> indices, const MeshLite& mesh)
{
    //  take the current number of vertices in the vertex buffer
    //  because the newly added indices will have to count from here
    auto vertexOffset = mVertexBufferData.size();
    //  take the current number of indices in the index buffer
    //  because the newly added surfaces will need to offset their starting index from here
    auto indexOffset = mIndexBufferData.size();
    //  vertices can be directly insert into the buffer
    mVertexBufferData.insert(mVertexBufferData.end(), vertices.begin(), vertices.end());
    //  indices need to add the offset first
    std::transform(indices.begin(), indices.end(), std::back_inserter(mIndexBufferData),
        [vertexOffset](IndexType index) { return index + vertexOffset; });
    mMeshes[mesh.mId] = mesh;
    for (SurfaceLite& surface : mMeshes[mesh.mId].mSurfaces)
    {
        surface.mFirstIndex += indexOffset;
    }
}
template <typename VertexType, typename IndexType>
inline void MeshBank<VertexType, IndexType>::buildMeshBuffers()
{
    const VkDeviceSize vertexSize = mVertexBufferData.size() * sizeof(VertexType);
    const VkDeviceSize indexSize = mIndexBufferData.size() * sizeof(IndexType);

    mVulkanResources->createAndMapBuffer(mVertexBufferData.data(), vertexSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VMA_ALLOCATION_CREATE_MAPPED_BIT, VMA_MEMORY_USAGE_GPU_ONLY, mVertexBuffer);
    mVulkanResources->createAndMapBuffer(mIndexBufferData.data(), indexSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        VMA_ALLOCATION_CREATE_MAPPED_BIT, VMA_MEMORY_USAGE_GPU_ONLY, mIndexBuffer);
}

template <typename VertexType, typename IndexType>
inline void MeshBank<VertexType, IndexType>::cleanup()
{
    mVulkanResources->destroyBuffer(mVertexBuffer);
    mVulkanResources->destroyBuffer(mIndexBuffer);
    mVertexBufferData.clear();
    mIndexBufferData.clear();
}
} // namespace Bunny::Render
