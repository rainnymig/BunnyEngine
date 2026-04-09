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
    // fragment coordination
    ivec2 coords = ivec2(gl_FragCoord.xy);

    // fragment revealage
    float revealage = texelFetch(revealageImage, coords, 0).r;

    // save the blending and color texture fetch cost if there is not a transparent fragment
    if (isApproximatelyEqual(revealage, 1.0f))
    {
        discard;
    }

    // fragment color
    vec4 accumulation = texelFetch(accumulationImage, coords, 0);

    // suppress overflow
    if (isinf(max3(abs(accumulation.rgb))))
    {
        accumulation.rgb = vec3(accumulation.a);
    }

    // prevent floating point precision bug
    vec3 average_color = accumulation.rgb / max(accumulation.a, EPSILON);

    // blend pixels
    outColor = vec4(average_color, 1.0f - revealage);
}