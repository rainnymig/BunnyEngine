#version 460

#include "pbrCommon.glsl"

layout(location = 0) in vec3 normal;
layout(location = 1) in vec3 fragPos;

layout (location = 0) out vec4 outColor;

layout(set = 1, binding = 0) uniform LightData
{
    Light lights[MAX_LIGHT_COUNT];
    uint lightCount;
};

layout(set = 1, binding = 1) uniform CameraData
{
    mat4 viewProj;
    mat4 inverseView;
    mat4 inverseProj;
    vec3 position;
    float exposure;
    vec2 resolution;
    float pad1;
    float pad2;
} cameraData;

void main()
{
    const vec3 color = vec3(0.341, 0.737, 1);
    Light light = lights[0];

    //  Blinn-Phong lighting model
    //  ambient
    float ambientStrength = 0.1f;
    vec3 ambientColor = light.color * ambientStrength;

    //  diffuse
    float diffuseStrength = 0.4f;
    vec3 dirToLight = (-light.dirOrPos).xyz;
    float diff = max(dot(normal, dirToLight), 0.0f);
    vec3 diffuseColor = light.color * diff * diffuseStrength;

    //  specular
    float specularStrength = 0.5f;
    vec3 dirToCamera = normalize(cameraData.position - fragPos);
    vec3 halfwayVec = normalize(dirToCamera + dirToLight);
    float spec = pow(max(dot(halfwayVec, normal), 0.0f), 64);
    vec3 specularColor = light.color * spec * specularStrength;

    //  combined
    vec3 result = (ambientColor + diffuseColor + specularColor) * color;
    outColor = vec4(result, 1.0f);
}
