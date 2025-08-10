#define DIRECTIONAL 0
#define POINT 1
#define MAX_LIGHT_COUNT 8

#define PI 3.14159265359

struct Light 
{
    vec3 dirOrPos;
    float intensity;    //  dir light - illuminance (lux or lumen/m2); point/spot light - luminous power (lumen)
    vec3 color;
    float influenceRadius;
    float innerAngle;
    float outerAngle;   
    uint type;
};

//  parameters: https://google.github.io/filament/Filament.html#materialsystem/parameterization
struct PbrMaterial
{
    vec4 baseColor;
    vec4 emissiveColor;
    float metallic;
    float roughness;
    float reflectance;
    float ambientOcclusion;
    uint colorTexId;
    uint normalTexId;
    uint emissiveTexId;
    uint metRouRflAmbTexId;
};

struct ObjectData
{
    mat4 model;
    mat4 invTransModel;
    vec3 scale;
    uint meshId;
    uint materialId;
    uint vertexOffset;
    uint firstIndex;
};

struct Vertex
{
    vec3 position;
    vec3 normal;
    vec3 tangent;
    vec4 color;
    vec3 texCoord;
};
