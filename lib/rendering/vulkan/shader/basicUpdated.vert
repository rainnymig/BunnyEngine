#version 450


/*//////

Descriptor Sets:
set 1: scene data
set 2: object data
set 3: material data

//////*/



/*  buffer layouts  */
layout(set = 0, binding = 0) uniform SceneData 
{
    mat4 view;
    mat4 proj;
    mat4 viewProj;
} sceneData;

layout(set = 1, binding = 0) uniform ObjectData 
{
    mat4 model;
    mat4 invTransModel;
} objectData;

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

    outNormal = normalize((objectData.invTransModel*vec4(normal, 0)).xyz);
    outColor = color.xyz;
    vec4 worldPos = objectData.model * pos;
    outFragPos = worldPos.xyz;
    outUV = uv;

    gl_Position = sceneData.viewProj * worldPos;
}