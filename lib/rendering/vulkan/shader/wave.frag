#version 460

#define WORLD_SET 2
#define MATERIAL_SET 3
#include "pbr.glsl"

layout(location = 0) in vec3 normal;
layout(location = 1) in vec3 fragPos;

layout(location = 0) out vec4 outColor;

layout(push_constant) uniform WavePushParams
{
    uint materialIdx;
};

void main()
{
    vec3 lightResult = vec3(0, 0, 0);

    for (uint i = 0; i < 1; i++)
    {
        lightResult += calculateLighting(materialInstances[materialIdx], lights[i], fragPos, 
                                            normalize(cameraData.position - fragPos), normal, mat3(1.0), vec2(0, 0));
    }

    lightResult *= cameraData.exposure;
    outColor = vec4(lightResult, 1.0);
}
