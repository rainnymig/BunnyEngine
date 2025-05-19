#version 450

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
};

/*  buffer layouts  */
layout(set = 0, binding = 2) uniform SceneData 
{
    mat4 view;
    mat4 proj;
    mat4 viewProj;
} sceneData;

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
layout (location = 2) in vec4 color;
layout (location = 3) in vec2 uv;

/*  output layouts  */
layout (location = 0) out vec3 outNormal;
layout (location = 1) out vec3 outColor;
layout (location = 2) out vec3 outFragPos;
layout (location = 3) out vec2 outUV;

void main()
{
    vec4 pos = vec4(position, 1);

    ObjectData objData = objectData[instToObj[gl_InstanceIndex]];

    outNormal = normalize((objData.invTransModel*vec4(normal, 0)).xyz);
    outColor = color.xyz;
    vec4 worldPos = objData.model * pos;
    outFragPos = worldPos.xyz;
    outUV = uv;

    gl_Position = sceneData.viewProj * worldPos;
}