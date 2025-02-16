#include "Mesh.h"

#include "Helper.h"
#include "Vertex.h"

#include <tiny_obj_loader.h>

namespace Bunny::Render
{
void MeshAssetsBank::destroyBank()
{
    assert(mRenderer != nullptr);

    for (auto& entry : mMeshes)
    {
        if (entry.second != nullptr)
        {
            mRenderer->destroyBuffer(entry.second->mVertexBuffer);
            mRenderer->destroyBuffer(entry.second->mIndexBuffer);
            entry.second.reset();
        }
    }
    mMeshes.clear();
}

} // namespace Bunny::Render