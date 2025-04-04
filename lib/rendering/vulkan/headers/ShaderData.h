#pragma once

#include "Light.h"

#include <glm/matrix.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace Bunny::Render
{
struct SceneData
{
    glm::mat4x4 mViewMatrix;
    glm::mat4x4 mProjMatrix;
    glm::mat4x4 mViewProjMatrix;
};

struct LightData
{
    static constexpr size_t MAX_LIGHT_COUNT = 8;
    glm::vec3 mCameraPos;
    uint32_t mLightCount;
    DirectionalLight mLights[MAX_LIGHT_COUNT];
};

struct ObjectData
{
    glm::mat4 model;
    glm::mat4 invTransModel;
};


//  object data for culling with bounding sphere
struct SphereCullObjectData
{
    glm::vec4 mBoundingSphere;  //  x, y, z: center, w: radius
    glm::mat4 mTransform;
    IdType mMeshId;
    IdType mSurfaceId;
};

//  surfaces of a mesh should have different material instances (or materials)
//  if multiple surfaces have same, they should be merged
//  mesh1: surface 11 - matInst1, surface12 - matInst2
//  mesh2: surface 21 - matInst1, surface22 - matInst3

//  game objects in world
//  {1, mesh1}
//  {2, mesh2}
//  {3, mesh1}
//  {4, mesh1}
//  {5, mesh1}
//  {6, mesh2}
//  {7, mesh1}
//  {8, mesh2}
//  {9, mesh2}

//  sorted game objects in world
//  {1, mesh1, surface 11, matInst1}
//  {3, mesh1, surface 11, matInst1}
//  {4, mesh1, surface 11, matInst1}
//  {7, mesh1, surface 11, matInst1}
//  {5, mesh1, surface 11, matInst1}
//  {1, mesh1, surface 12, matInst2}
//  {3, mesh1, surface 12, matInst2}
//  {4, mesh1, surface 12, matInst2}
//  {7, mesh1, surface 12, matInst2}
//  {5, mesh1, surface 12, matInst2}
//  {2, mesh2, surface 21, matInst1}
//  {8, mesh2, surface 21, matInst1}
//  {9, mesh2, surface 21, matInst1}
//  {6, mesh2, surface 21, matInst1}
//  {2, mesh2, surface 22, matInst3}
//  {8, mesh2, surface 22, matInst3}
//  {9, mesh2, surface 22, matInst3}
//  {6, mesh2, surface 22, matInst3}

//  {1, mesh1}
//  {3, mesh1}
//  {4, mesh1}
//  {7, mesh1}
//  {5, mesh1}
//  {2, mesh2}
//  {8, mesh2}
//  {9, mesh2}
//  {6, mesh2}

//  constant buffers in culler
//  bounds: [boundingMesh1, boundingMesh2, ...]

//  culler input
//  {transform1, mesh1Id}
//  {transform3, mesh1Id}
//  {transform4, mesh1Id}
//  {transform7, mesh1Id}
//  {transform5, mesh1Id}
//  {transform2, mesh2Id}
//  {transform8, mesh2Id}
//  {transform9, mesh2Id}
//  {transform6, mesh2Id}

//  culler output buffer object and mat data
//  {transform1, mesh1Id}
//  {transform3, mesh1Id}
//  {transform2, mesh2Id}
//  {transform8, mesh2Id}
//  {transform9, mesh2Id}

//  culler output buffer VkDrawIndexedIndirectCommand
//  { indexCountSurface11, instanceCountMesh1, firstIndexSurface11, vertexOffsetMesh1, firstInstance(0) }
//  { indexCountSurface12, instanceCountMesh1, firstIndexSurface12, vertexOffsetMesh1, firstInstance(sum of prev) }
//  { indexCountSurface21, instanceCountMesh2, firstIndexSurface21, vertexOffsetMesh2, firstInstance(sum of prev) }
//  { indexCountSurface22, instanceCountMesh2, firstIndexSurface22, vertexOffsetMesh2, firstInstance(sum of prev) }

//  vertex shader input
//  per vertex                  - from vertex buffer
//      position
//      normal
//      color
//      texcoord
//  per instance (object)       - from uniform buffer through instance id
//      transform matrix
//      inversed transposed transform matrix
//      matInstId
//  
//  pixel shader input
//  per material instance       - from uniform buffer through material instance id
//      material parameters
//      material textures (or texture-sampler combis)
//      

struct CullResultData
{
    glm::mat4 mTransform;
    glm::mat4 mInversedTransposedTransform;
};

} // namespace Bunny::Render
