#version 460

layout(set = 0, binding = 0) uniform sampler2D renderedSceneTexture;
layout(set = 0, binding = 1) uniform sampler2D cloudTexture;
layout(set = 0, binding = 2) uniform sampler2D fogShadowTexture;

layout (location = 0) in vec2 uv;

layout (location = 0) out vec4 outColor;

void main()
{
    vec3 sceneTex = texture(renderedSceneTexture, uv).xyz;
    vec4 cloudTex = texture(cloudTexture, uv);
    vec2 fogShadowTex = texture(fogShadowTexture, uv).xy;
    sceneTex *= fogShadowTex.y;
    vec3 col = sceneTex * (cloudTex.w) * (1 - fogShadowTex.x) + cloudTex.xyz;

    outColor = vec4(col/(col+1.0), 1.0);
    // outColor = vec4(col, 1.0);
    // outColor = cloudTex;

    // outColor = texture(renderedSceneTexture, uv);

}