#version 460

layout(set = 0, binding = 0) uniform sampler2D accumulationImage;
layout(set = 0, binding = 1) uniform sampler2D revealageImage;

layout (location = 0) out vec4 outColor;

const float EPSILON = 0.00001f;
bool isApproximatelyEqual(float a, float b)
{
    return abs(a - b) <= (abs(a) < abs(b) ? abs(b) : abs(a)) * EPSILON;
}

float max3(vec3 v)
{
    return max(max(v.x, v.y), v.z);
}

//  from https://learnopengl.com/Guest-Articles/2020/OIT/Weighted-Blended
void main()
{
    ivec2 coords = ivec2(gl_FragCoord.xy);

    float revealage = texelFetch(revealageImage, coords, 0).r;

    if (isApproximatelyEqual(revealage, 1.0f))
    {
        discard;
    }

    vec4 accumulation = texelFetch(accumulationImage, coords, 0);

    if (isinf(max3(abs(accumulation.rgb))))
    {
        accumulation.rgb = vec3(accumulation.a);
    }

    vec3 averageColor = accumulation.rgb / max(accumulation.a, EPSILON);

    outColor = vec4(averageColor, 1.0f - revealage);
}