#version 460

layout(set = 0, binding = 0) uniform sampler2D renderedSceneTexture;
layout(set = 0, binding = 1) uniform sampler2D cloudTexture;
layout(set = 0, binding = 2) uniform sampler2D fogShadowTexture;

layout (location = 0) in vec2 uv;

layout (location = 0) out vec4 outColor;

float remap(float v, float x0, float y0, float x1, float y1)
{
	return x1 + ((v - x0) / (y0 - x0)) * (y1 - x1);
}

void main()
{
    vec3 sceneTex = texture(renderedSceneTexture, uv).xyz;

    vec4 cloudTex = texture(cloudTexture, uv);
    vec2 fogShadowTex = texture(fogShadowTexture, uv).xy;
    float shadowCoef = remap(fogShadowTex.y, 0, 1, 0.75, 1);
    sceneTex *= shadowCoef;
    vec3 correctedCloudColor = cloudTex.xyz/(cloudTex.xyz+1.0);
    vec3 col = sceneTex * (cloudTex.w) * (1 - fogShadowTex.x) + correctedCloudColor;

    // outColor = vec4(col/(col+1.0), 1.0);
    outColor = vec4(col, 1.0);
}