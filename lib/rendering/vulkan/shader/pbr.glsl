#define DIRECTIONAL 1
#define POINT 2
#define MAX_LIGHT_COUNT 8

struct Light 
{
    vec3 dirOrPos;
    float intensity;    //  temp name, will have different meaning for point and directional light
    vec3 color;
    uint type;
};

//  update every frame
layout(set = 0, binding = 0) uniform LightData
{
    Light lights[MAX_LIGHT_COUNT];
    uint lightCount;
};

layout(set = 0, binding = 1) uniform CameraData
{
    vec3 position;
} cameraData;

struct PbrMaterial
{
    vec4 baseColor;
    vec3 emissiveColor;
    float metallic;
    float roughness;
    float reflectance;
    float ambientOcclusion;
    uint mColorTexId;
    uint mNormalTexId;
    uint mEmissiveTexId;
    uint mMetRouRflAmbTexId;
};

//  update irregularly, maybe only once at start if materials don't change
layout(std430, set = 2, binding = 0) buffer MaterialData
{
    PbrMaterial materialInstances[];
};

vec3 calculateLighting(PbrMaterial material, Light light, vec3 fragPos, vec3 viewDir, vec3 normal)
{
    vec3 result = material.baseColor;

    return result;
}