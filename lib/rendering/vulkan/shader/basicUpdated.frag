#version 450

struct Light 
{
    vec3 direction;
    vec3 color;
};

layout(set = 0, binding = 1) uniform LightData 
{
    Light lights[];
    vec3 cameraPos;
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
    float diffuseStrength = 0.5f;
    vec3 dirToLight = (-light.direction).xyz;
    float diff = max(dot(normal, dirToLight), 0.0f);
    vec3 diffuseColor = light.color * diff * diffuseStrength;

    //  specular
    vec3 dirToCamera = normalize(lightData.cameraPos - fragPos);
    vec3 halfwayVec = normalize(dirToCamera + dirToLight);
    float spec = pow(max(dot(halfwayVec, normal), 0.0f), 32);
    float specularStrength = 0.5f;
    vec3 specularColor = light.color * spec * specularStrength;


    //  combined
    vec3 result = (ambientColor + diffuseColor + specularColor) * color;
    outColor = vec4(result, 1.0f);
}