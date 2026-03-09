#define DIRECTIONAL 0
#define POINT 1
#define MAX_LIGHT_COUNT 8

#define PI 3.14159265359

#define INVALID_ID 114514

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
    uint metalRoughnessTexId;
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

struct BoundingSphere
{
    vec3 center;
    float radius;
};

struct SurfaceData
{
    uint vertexOffset;
    uint materialId;
    uint firstIndex;
};

struct MeshData
{
    BoundingSphere bounds;
    uint firstSurface;
    uint surfaceCount;
    uint padding1;
    uint padding2;
};

struct Vertex
{
    vec4 position;
    vec4 normal;
    vec4 tangent;
    vec3 texCoord;
    uint surfaceIdx;    //  the index of surface in the mesh (not in the surface data array)
};
