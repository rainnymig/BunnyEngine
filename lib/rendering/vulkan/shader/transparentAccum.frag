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
layout(set = 3, binding = 1) uniform sampler2D sceneDepth;

layout(push_constant) uniform SceneInfo
{
    float zNear;
    float zFar;
    float zFactor;      //  the z-based factor gives precedence to nearer surfaces
};

float recoverLinearDepth(float nonLinearDepth)
{
    //  check zNear and zFar direction
	return -(zNear * zFar) / ((zFar - zNear) * nonLinearDepth - zFar);
}

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

    float depth = imageLoad(sceneDepth, ivec2(gl_FragCoord.xy)).x;
    float z = recoverLinearDepth(depth);
    float weight = max(min(1.0, max(max(lightResult.r, lightResult.g), lightResult.b) * alpha), alpha) * 
                    clamp(0.03 / (1e-5 + pow(z / zFactor, 4.0)), 1e-2, 3e3);

    outAccumulation = vec4(lightResult * alpha, alpha) * weight;
    outRevealage = alpha;

    // outColor = vec4(lightResult, 1);
}