#include "Renderable.h"

#include "Mesh.h"

#include <glm/mat4x4.hpp>
#include <glm/ext/matrix_transform.hpp>

namespace Bunny::Render
{
void Scene::render(VkCommandBuffer commandBuffer)
{
}

MeshRenderComponent::MeshRenderComponent(const Mesh* mesh) : mMesh(mesh)
{
}

void MeshRenderComponent::render(VkCommandBuffer commandBuffer)
{
    //  calculate transform matrix
    glm::mat4 translateMat = glm::translate(glm::mat4(1.0f), mTransform.mPosition);
    glm::mat4 rotationMat = glm::toMat4(mTransform.mRotation);
    glm::mat4 scaleMat = glm::scale(glm::mat4(1.0f), mTransform.mScale);

    glm::mat4 modelMat = translateMat * rotationMat * scaleMat;
    glm::mat4 invTransMat = glm::inverse(glm::transpose(modelMat));

    //  bind buffers
    VkBuffer vertexBuffers[] = {mMesh->mVertexBuffer.mBuffer};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

    vkCmdBindIndexBuffer(commandBuffer, mMesh->mIndexBuffer.mBuffer, 0, VK_INDEX_TYPE_UINT32);

    RenderPushConstant pushConst{.model = modelMat, .invTransModel = invTransMat};

    //  draw!!
    for (const auto& surface : mMesh->mSurfaces)
    {
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, surface.mMaterial.mpBaseMaterial->mPipiline);
        vkCmdPushConstants(commandBuffer, surface.mMaterial.mpBaseMaterial->mPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT,
            0, sizeof(RenderPushConstant), &pushConst);
        vkCmdDrawIndexed(commandBuffer, surface.mCount, 1, surface.mStartIndex, 0, 0);
    }
}

} // namespace Bunny::Render
