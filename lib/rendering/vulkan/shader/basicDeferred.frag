#version 450

struct Light 
{
    vec3 direction;
    float pad1;
    vec3 color;
    float pad2;
};

#define MAX_LIGHT_COUNT 8

layout(set = 0, binding = 0) uniform LightData 
{
    vec3 cameraPos;
    uint lightCount;
    Light lights[MAX_LIGHT_COUNT];
} lightData;

layout(set = 1, binding = 0) uniform sampler2D colorMap;
layout(set = 1, binding = 1) uniform sampler2D fragPosTexUMap;
layout(set = 1, binding = 2) uniform sampler2D normalTexVMap;

layout (location = 0) in vec2 uv;

layout (location = 0) out vec4 outColor;

void main()
{
    // outColor = vec4(uv.x, uv.y, 1, 1);

    vec3 color = texture(colorMap, uv).rgb;
    vec4 fragPosTexU = texture(fragPosTexUMap, uv);
    vec3 fragPos = fragPosTexU.xyz;
    vec4 normalTexV = texture(normalTexVMap, uv);
    vec3 normal = vec3(normalTexV.xyz);
    vec2 texCoord = vec2(fragPosTexU.w, normalTexV.w);

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