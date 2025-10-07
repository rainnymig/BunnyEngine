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
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec3 tangent;
layout (location = 3) in vec4 color;
layout (location = 4) in vec3 uv;
layout (location = 5) in uint surfaceIndex;

/*  output layouts  */
layout (location = 0) out vec3 outNormal;
layout (location = 1) out vec3 outColor;
layout (location = 2) out vec3 outFragPos;
layout (location = 3) out vec2 outUV;
layout (location = 4) out uint outMatId;
layout (location = 5) out mat3 outTbnMat;   //  tangent-bitangent-normal matrix
                                            //  maybe calculate this in frag shader instead to allow correct interpolation?

void main()
{
    vec4 pos = vec4(position, 1);

    ObjectData objData = objectData[instToObj[gl_InstanceIndex]];
    MeshData mesh = meshData[objData.meshId];
    SurfaceData surface = surfaceData[mesh.firstSurface + surfaceIndex];

    outNormal = normalize((objData.invTransModel * vec4(normal, 0)).xyz);
    outColor = color.xyz;
    vec4 worldPos = objData.model * pos;
    outFragPos = worldPos.xyz;
    outUV = uv.xy;
    outMatId = surface.materialId;

    vec3 tangentTransformed = normalize((objData.invTransModel * vec4(tangent, 0)).xyz);
    vec3 bitangentTransformed = normalize(cross(outNormal, tangentTransformed));

    outTbnMat = mat3(tangentTransformed, bitangentTransformed, outNormal);

    gl_Position = cameraData.viewProj * worldPos;
}