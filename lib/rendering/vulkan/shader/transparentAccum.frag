#version 460

#include "pbr.glsl"

layout (location = 0) in vec3 normal;
layout (location = 1) in vec3 fragPos;
layout (location = 2) in vec2 uv;
layout (location = 3) flat in uint matId;
layout (location = 4) in vec3 tangent;
layout (location = 5) in vec3 bitangent;

layout (location = 0) out vec4 outAccumulation;
layout (location = 1) out float outRevealage;

layout(set = 3, binding = 0, r32ui) uniform readonly uimage2D lightShadowMap;

void main()
{
    uint lightCountCapped = min(lightCount, MAX_LIGHT_COUNT);
    vec3 lightResult = vec3(0, 0, 0);
    const uint shadowBit = 1;
    uvec4 lightShadowInfo = imageLoad(lightShadowMap, ivec2(gl_FragCoord.xy));

    mat3 tbnMat = mat3(tangent, bitangent, normal);

    float alpha = 1;

    for (uint i = 0; i < lightCountCapped; i++)
    {
        float shadowCoef = (lightShadowInfo.r & (shadowBit << i)) > 0 ? 0.5 : 1.0;
        vec4 pbrLightCalcResult = calculateLighting(materialInstances[matId], lights[i], fragPos, 
                                            normalize(cameraData.position - fragPos), normal, tbnMat, uv);
        lightResult += shadowCoef * pbrLightCalcResult.rgb;

        alpha = pbrLightCalcResult.a;
    }
    lightResult *= cameraData.exposure;

    //  weight function
    //  more in https://jcgt.org/published/0002/02/09/
    float weight = clamp(pow(min(1.0, alpha* 10.0) + 0.01, 3.0) * 1e8 * 
                         pow(1.0 - gl_FragCoord.z * 0.9, 3.0), 1e-2, 3e3);

    outAccumulation = vec4(lightResult * alpha, alpha) * weight;
    outRevealage = alpha;

    // outColor = vec4(lightResult, 1);
}