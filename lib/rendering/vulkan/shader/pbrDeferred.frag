#version 450

#include "pbr.glsl"

//  gbuffer images
layout(set = 1, binding = 0) uniform sampler2D colorMap;
layout(set = 1, binding = 1) uniform sampler2D fragPosTexUMap;
layout(set = 1, binding = 2) uniform sampler2D normalTexVMap;

layout (location = 0) in vec2 uv;

layout (location = 0) out vec4 outColor;

void main()
{
    vec4 fragPosTexU = texture(fragPosTexUMap, uv);
    vec3 fragPos = fragPosTexU.xyz;
    vec4 normalTexV = texture(normalTexVMap, uv);
    vec3 normal = vec3(normalTexV.xyz);
    vec2 texCoord = vec2(fragPosTexU.w, normalTexV.w);

    uint lightCountCapped = min(lightCount, MAX_LIGHT_COUNT);
    vec3 lightResult = vec3(0, 0, 0);
    for (uint i = 0; i < lightCountCapped; i++)
    {
        lightResult += calculateLighting(materialInstances[0], lights[0], fragPos, normalize(cameraData.position - fragPos), normal);
    }
    outColor = vec4(lightResult, 1);
}
