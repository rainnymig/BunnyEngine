#version 460

#extension GL_EXT_ray_query : require

#define WORLD_SET 2
#define MATERIAL_SET 3
#include "pbr.glsl"

layout(set = 1, binding = 0) uniform sampler2D waveDisplacementTex;
layout(set = 1, binding = 1) uniform sampler2D waveNormalTex;

//  top level acceleration structure for ray traced shadow
layout(set = 4, binding = 0) uniform accelerationStructureEXT topLevelAcceStruct;

// layout(location = 0) in vec3 normal;
layout(location = 0) in vec2 texCoord;
layout(location = 1) in vec3 fragPos;

layout(location = 0) out vec4 outColor;

layout(push_constant) uniform WavePushParams
{
    uint materialIdx;
};

vec3 getWaveDisplacement(vec2 normalizedWorldPosXZ)
{
    return texture(waveDisplacementTex, normalizedWorldPosXZ).xyz;
}

vec3 getWaveNormal(vec2 normalizedWorldPosXZ)
{
    vec3 norm = texture(waveNormalTex, normalizedWorldPosXZ).xyz;
    return norm * 2.0 - 1.0;
    // return norm;
}

void main()
{
    vec3 lightResult = vec3(0, 0, 0);

    vec3 normal = getWaveNormal(texCoord);
    // outColor = vec4(normal, 1.0);
    // outColor = vec4(texCoord.x, texCoord.y, 0, 1);

    for (uint i = 0; i < 1; i++)
    {
        float shadowCoef = 1.0;
        
        float lightDistance = 10000.0;
        vec3 dirToLight;
        if (lights[i].type == DIRECTIONAL)
        {
            dirToLight = -lights[i].dirOrPos;
        }
        else
        {
            vec3 vecToLight = lights[i].dirOrPos - fragPos;
            lightDistance = length(vecToLight);
            dirToLight = normalize(vecToLight);
        }

        if (dot(normal, dirToLight) > 0)
        {
            rayQueryEXT rayQuery;
            rayQueryInitializeEXT(
                rayQuery,                           // query object
                topLevelAcceStruct,                 // TLAS
                gl_RayFlagsTerminateOnFirstHitEXT   // flags — fast shadow check
                    | gl_RayFlagsOpaqueEXT,
                0xFF,                               // cull mask
                fragPos,                            // origin
                0.001,                              // tMin
                dirToLight,                         // direction
                lightDistance                       // tMax
            );

            while (rayQueryProceedEXT(rayQuery)) {
                if (rayQueryGetIntersectionTypeEXT(rayQuery, false)
                        == gl_RayQueryCandidateIntersectionTriangleEXT) {
                    rayQueryConfirmIntersectionEXT(rayQuery);
                }
            }

            if (rayQueryGetIntersectionTypeEXT(rayQuery, true)
                    == gl_RayQueryCommittedIntersectionTriangleEXT) {
                shadowCoef = 0.2; // occluded
            }
        }

        lightResult += shadowCoef * calculateLighting(materialInstances[materialIdx], lights[i], fragPos, 
                                            normalize(cameraData.position - fragPos), normal, mat3(1.0), vec2(0, 0));
    }

    lightResult *= cameraData.exposure;
    outColor = vec4(lightResult, 1.0);
}
