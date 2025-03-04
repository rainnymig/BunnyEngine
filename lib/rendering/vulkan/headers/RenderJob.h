#pragma once

#include "Fundamentals.h"

namespace Bunny::Render
{

//  render passes:
//  deferred rendering:
//  culling (compute) - all objects, bounding box, transform, view frustum, light frustums
//  shadow - culled objects, mesh, transform, shadow casting lights
//  gbuffer - culled objects, mesh, transform, material id, shadow map, light transform for shadow testing
//  deferred lighting - gbuffer, materials
//  post proc - lighted image
//
//  or
//  forward rendering:
//  culling (compute) - all objects, bounding box, transform, view frustum, light frustums
//  shadow - culled objects, mesh, transform, shadow casting lights
//  forward rendering, batched by material/material/mesh instance
//  post proc - lighted image

struct RenderBatch
{
    IdType mId;
    IdType mMaterialId;         //  gets material pipeline
    IdType mMaterialInstanceId; //  gets material descriptor sets
    IdType mMeshId;             //  gets vertices and indices
    //  may need startIndex and indexCount for a surface, since a mesh can have surfaces of different materials

    AllocatedBuffer mObjectBuffer; //  should hold an array of ObjectData for rendering
                                   //  i.e. should be used directly as uniform buffer
};
} // namespace Bunny::Render
