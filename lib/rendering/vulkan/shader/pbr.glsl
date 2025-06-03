#extension GL_EXT_nonuniform_qualifier : require

#define DIRECTIONAL 1
#define POINT 2
#define MAX_LIGHT_COUNT 8

#define PI 3.14159265359

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

//  update irregularly, maybe only once at start if materials don't change
layout(std430, set = 2, binding = 0) buffer MaterialData
{
    PbrMaterial materialInstances[];
};

layout(set = 2, binding = 1) uniform sampler2D textures[];

//  normal distribution function (specular D)
//  ndoth: normal dot halfway vector
float d_ggx(float ndoth, float roughness) {
    float a = ndoth * roughness;
    float k = roughness / (1.0 - ndoth * ndoth + a * a);
    return k * k * (1.0 / PI);
}

//  visibility term (specular V)
float v_smithGgxCorrelated(float ndotv, float ndotl, float roughness)
{
    float a2 = roughness * roughness;
    float ggxV = ndotl * sqrt(ndotv * ndotv * (1.0 - a2) + a2);
    float ggxL = ndotv * sqrt(ndotl * ndotl * (1.0 - a2) + a2);
    return 0.5 / (ggxV + ggxL);
}

//  visibility term fast (specular V)
float v_smithGgxCorrelatedFast(float ndotv, float ndotl, float roughness)
{
    float a = roughness;
    float ggxV = ndotl * (ndotv * (1.0 - a) + a);
    float ggxL = ndotv * (ndotl * (1.0 - a) + a);
    return 0.5 / (ggxV + ggxL);
}

//  fresnel (specular F)
//  f90 == 1
vec3 f_schlick(float u, vec3 f0)
{
    float f = pow(1.0 - u, 5.0);
    return f + f0 * (1.0 - f);
}

//  diffuse BRDF
//  Lambertian - uniform diffuse response over the microfacets hemisphere
float fd_lambert()
{
    return 1.0 / PI;
}

//  mostly based on the implementation in Filament: https://google.github.io/filament/Filament.html
vec3 calculateLighting(PbrMaterial material, Light light, vec3 fragPos, vec3 viewDir, vec3 normalFromVtx, vec2 texCoord)
{
    vec4 baseColor;
    vec4 emissiveColor;
    vec3 normal;
    float metallic;
    float roughness;
    float reflectance;
    float ambientOcclusion;

    //  decide whether to use values from texture
    //  the metallic value is set to less than 0 if want to use texture
    if (material.metallic < 0)
    {
        baseColor = texture(textures[material.colorTexId], texCoord);
        emissiveColor = texture(textures[material.emissiveTexId], texCoord);
        normal = texture(textures[material.normalTexId], texCoord).xyz;     //  actually need to tranform from local to world coordinate
        vec4 mrra = texture(textures[material.metRouRflAmbTexId], texCoord);
        metallic = mrra.x;
        roughness = mrra.y;
        reflectance = mrra.z;
        ambientOcclusion = mrra.w;
    }
    else
    {
        baseColor = material.baseColor;
        emissiveColor = material.emissiveColor;
        normal = normalFromVtx;
        metallic = material.metallic;
        roughness = material.roughness;
        reflectance = material.reflectance;
        ambientOcclusion = material.ambientOcclusion;
    }

    vec3 lightDir = light.type == DIRECTIONAL ? -light.dirOrPos : normalize(light.dirOrPos - fragPos);
    vec3 halfVec = normalize(lightDir + viewDir);

    float ndotv = abs(dot(normal, viewDir)) + 1e-5;     //  adding a small value to prevent it being 0 at grazing view angle
    float ndotl = clamp(dot(normal, lightDir), 0.0, 1.0);
    float ndoth = clamp(dot(normal, halfVec), 0.0, 1.0);
    float ldoth = clamp(dot(lightDir, halfVec), 0.0, 1.0);

    float D = d_ggx(ndoth, roughness);

    vec3 f0 = 0.16 * reflectance * reflectance * (1.0 - metallic) + baseColor * metallic;
    float F = f_schlick(ldoth, f0);

    float V = v_smithGgxCorrelated(ndotv, ndotl, roughness);

    //  specular component
    vec3 Fr = D * V * F;

    //  diffuse component
    vec3 diffuseColor = (1.0 - metallic) * baseColor.rgb;
    vec3 Fd = diffuseColor * fd_lambert();

    vec3 result = material.baseColor.xyz;
    return result;
}