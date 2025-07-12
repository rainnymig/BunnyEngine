#version 460

layout (location = 0) in vec3 position;
layout (location = 1) in vec2 uv;

layout (location = 0) out vec2 outUv;

layout(set = 0, binding = 2) uniform SceneData 
{
    mat4 view;
    mat4 proj;
    mat4 viewProj;
} sceneData;

void main()
{
    outUv = uv;
    gl_Position = vec4(position, 1.0);
}