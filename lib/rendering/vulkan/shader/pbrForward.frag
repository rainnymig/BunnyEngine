#version 460

#include "pbr.glsl"

layout (location = 0) in vec3 normal;
layout (location = 1) in vec3 color;
layout (location = 2) in vec3 fragPos;
layout (location = 3) in vec2 uv;
layout (location = 4) flat in uint matId;
layout (location = 5) flat in mat3 tbnMat;

layout (location = 0) out vec4 outColor;

void main()
{
    uint lightCountCapped = min(lightCount, MAX_LIGHT_COUNT);
    vec3 lightResult = vec3(0, 0, 0);
    for (uint i = 0; i < lightCountCapped; i++)
    {
        lightResult += calculateLighting(materialInstances[matId], lights[i], fragPos, 
                                            normalize(cameraData.position - fragPos), normal, tbnMat, uv);
    }
    lightResult *= cameraData.exposure;
    outColor = vec4(lightResult, 1);
    // outColor = vec4(1, 1, 1, 1);
}