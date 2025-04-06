#version 450

struct Light 
{
    vec3 direction;
    float pad1;
    vec3 color;
    float pad2;
};

#define MAX_LIGHT_COUNT 8

layout(set = 0, binding = 1) uniform LightData 
{
    vec3 cameraPos;
    uint lightCount;
    Light lights[MAX_LIGHT_COUNT];
} lightData;

layout (location = 0) in vec3 normal;
layout (location = 1) in vec3 color;
layout (location = 2) in vec3 fragPos;
layout (location = 3) in vec2 uV;

layout (location = 0) out vec4 outColor;

void main()
{
    Light light = lightData.lights[0];

    //  Blinn-Phong lighting model

    //  ambient
    float ambientStrength = 0.1f;
    vec3 ambientColor = light.color * ambientStrength;

    //  diffuse
    float diffuseStrength = 0.4f;
    vec3 dirToLight = (-light.direction).xyz;
    float diff = max(dot(normal, dirToLight), 0.0f);
    vec3 diffuseColor = light.color * diff * diffuseStrength;

    //  specular
    float specularStrength = 0.8f;
    vec3 dirToCamera = normalize(lightData.cameraPos - fragPos);
    vec3 halfwayVec = normalize(dirToCamera + dirToLight);
    float spec = pow(max(dot(halfwayVec, normal), 0.0f), 64);
    vec3 specularColor = light.color * spec * specularStrength;

    //  combined
    vec3 result = (ambientColor + diffuseColor + specularColor) * color;
    // vec3 result = light.color * spec;
    // vec3 result = color;
    outColor = vec4(result, 1.0f);
}