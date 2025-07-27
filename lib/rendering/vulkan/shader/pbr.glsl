#extension GL_EXT_nonuniform_qualifier : require

#include "pbrCommon.glsl"

//  update every frame
layout(set = 0, binding = 0) uniform LightData
{
    Light lights[MAX_LIGHT_COUNT];
    uint lightCount;
};

layout(set = 0, binding = 1) uniform CameraData
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

//  update irregularly, maybe only once at start if materials don't change
layout(std430, set = 2, binding = 0) readonly buffer MaterialData
{
    PbrMaterial materialInstances[];
};

layout(set = 2, binding = 1) uniform sampler2D textures[];

//  normal distribution function (specular D)
//  ndoth: normal dot halfway vector
float d_ggx(float ndoth, float alpha) {
    float a = ndoth * alpha;
    float k = alpha / (1.0 - ndoth * ndoth + a * a);
    return k * k * (1.0 / PI);
}

//  visibility term (specular V)
float v_smithGgxCorrelated(float ndotv, float ndotl, float alpha)
{
    float a2 = alpha * alpha;
    float ggxV = ndotl * sqrt(ndotv * ndotv * (1.0 - a2) + a2);
    float ggxL = ndotv * sqrt(ndotl * ndotl * (1.0 - a2) + a2);
    return 0.5 / (ggxV + ggxL);
}

//  visibility term fast (specular V)
float v_smithGgxCorrelatedFast(float ndotv, float ndotl, float alpha)
{
    float a = alpha;
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

//  BRDF
vec3 brdf(vec3 baseColor, vec3 normal, vec3 lightDir, vec3 viewDir, float metallic, float roughness, float reflectance)
{
    vec3 halfVec = normalize(lightDir + viewDir);
    float ndotv = abs(dot(normal, viewDir)) + 1e-5;     //  adding a small value to prevent it being 0 at grazing view angle
    float ndotl = clamp(dot(normal, lightDir), 0.0, 1.0);
    float ndoth = clamp(dot(normal, halfVec), 0.0, 1.0);
    float ldoth = clamp(dot(lightDir, halfVec), 0.0, 1.0);

    //  map from perceptual roughness to actual roughness
    float alpha = roughness * roughness;
    vec3 f0 = 0.16 * reflectance * reflectance * (1.0 - metallic) + baseColor * metallic;
    vec3 diffuseColor = (1.0 - metallic) * baseColor;

    float D = d_ggx(ndoth, alpha);
    vec3 F = f_schlick(ldoth, f0);
    float V = v_smithGgxCorrelated(ndotv, ndotl, alpha);

    //  specular component
    vec3 Fr = D * V * F;

    //  Fr multi scattering compensation
    //  ?

    //  diffuse component
    vec3 Fd = diffuseColor * fd_lambert();

    return Fr + Fd;
}

float getSquareFalloffAttenuation(vec3 posToLight, float invRadius)
{
    float distanceSquare = dot(posToLight, posToLight);
    float factor = distanceSquare * invRadius * invRadius;
    float smoothFactor = max(1.0 - factor * factor, 0.0);
    return (smoothFactor * smoothFactor) / max(distanceSquare, 1e-4);
}

//  mostly based on the implementation in Filament: https://google.github.io/filament/Filament.html
vec3 calculateLighting(PbrMaterial material, Light light, vec3 fragPos, vec3 viewDir, vec3 normalFromVtx, mat3 tbnMatrix, vec2 texCoord)
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
        normal = normalize(tbnMatrix * texture(textures[material.normalTexId], texCoord).xyz);
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

    vec3 f = brdf(baseColor.rgb, normal, lightDir, viewDir, metallic, roughness, reflectance);

    //  calculate luminance
    float ndotl = clamp(dot(normal, lightDir), 0.0, 1.0);
    vec3 outLuminance;
    if (light.type == DIRECTIONAL)
    {
        float illuminance = light.intensity * ndotl;
        outLuminance = f * illuminance * light.color;
    }
    else if (light.type == POINT)
    {
        vec3 posToLight =  light.dirOrPos - fragPos;
        float attenuation = getSquareFalloffAttenuation(posToLight, 1.0 / light.influenceRadius);
        outLuminance = f * light.intensity * attenuation * ndotl * light.color;
    }

    //  camera exposure will be applied with all light results combined
    return outLuminance;
}