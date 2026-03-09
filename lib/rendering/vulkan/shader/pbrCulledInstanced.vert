#version 460

#include "pbr.glsl"

/*//////
Descriptor Sets:
set 0: scene data
set 1: object data
set 2: material and mesh data
//////*/

layout(std430, set = 1, binding = 0) buffer ObjectDataBuffer
{
    ObjectData objectData[];
};

layout(std430, set = 1, binding = 1) buffer InstanceToObjectId
{
    uint instToObj[];
};

/*  input layouts   */
layout (location = 0) in vec4 position;
layout (location = 1) in vec4 normal;
layout (location = 2) in vec4 tangent;
layout (location = 3) in vec3 uv;
layout (location = 4) in uint surfaceIndex;

/*  output layouts  */
layout (location = 0) out vec3 outNormal;
layout (location = 1) out vec3 outFragPos;
layout (location = 2) out vec2 outUV;
layout (location = 3) out flat uint outMatId;
layout (location = 4) out vec3 outTangent;
layout (location = 5) out vec3 outBitangent;

void main()
{
    ObjectData objData = objectData[instToObj[gl_InstanceIndex]];
    MeshData mesh = meshData[objData.meshId];
    SurfaceData surface = surfaceData[mesh.firstSurface + surfaceIndex];

    outNormal = normalize((objData.invTransModel * normal).xyz);
    vec4 worldPos = objData.model * position;
    outFragPos = worldPos.xyz;
    outUV = uv.xy;
    outMatId = surface.materialId;

    outTangent = normalize((objData.invTransModel * tangent).xyz);
    outBitangent = normalize(cross(outNormal, outTangent));

    gl_Position = cameraData.viewProj * worldPos;
}