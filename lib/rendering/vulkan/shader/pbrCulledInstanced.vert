#version 450

#include "pbr.glsl"

/*//////

Descriptor Sets:
set 0: scene data
set 1: object data
set 2: material data

//////*/

struct ObjectData
{
    mat4 model;
    mat4 invTransModel;
    vec3 scale;
    uint meshId;
    uint materialId;
};

/*  buffer layouts  */
// layout(set = 0, binding = 2) uniform SceneData 
// {
//     mat4 view;
//     mat4 proj;
//     mat4 viewProj;
// } sceneData;

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
layout (location = 4) in vec2 uv;

/*  output layouts  */
layout (location = 0) out vec3 outNormal;
layout (location = 1) out vec3 outColor;
layout (location = 2) out vec3 outFragPos;
layout (location = 3) out vec3 outUVMatId;
layout (location = 4) out mat3 outTbnMat;   //  tangent-bitangent-normal matrix

void main()
{
    vec4 pos = vec4(position, 1);

    ObjectData objData = objectData[instToObj[gl_InstanceIndex]];

    outNormal = normalize((objData.invTransModel * vec4(normal, 0)).xyz);
    outColor = color.xyz;
    vec4 worldPos = objData.model * pos;
    outFragPos = worldPos.xyz;
    outUVMatId = vec3(uv.x, uv.y, objData.materialId);

    vec3 tangentTransformed = normalize((objData.invTransModel * vec4(tangent, 0)).xyz);
    vec3 bitangentTransformed = normalize(cross(outNormal, tangentTransformed));

    outTbnMat = mat3(tangentTransformed, bitangentTransformed, outNormal);

    gl_Position = cameraData.viewProj * worldPos;
}