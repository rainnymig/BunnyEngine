#version 450

layout (location = 0) in vec3 normal;
layout (location = 1) in vec3 color;
layout (location = 2) in vec3 fragPos;
layout (location = 3) in vec2 uv;

layout (location = 0) out vec4 outColor;
layout (location = 1) out vec4 outFragPosTexU;
layout (location = 2) out vec4 outNormalTexV;
//  layout out material intance id?

void main()
{
    outColor = vec4(color, 1.0);
    outFragPosTexU = vec4(fragPos, uv.x);
    outNormalTexV = vec4(normal, uv.y);
}