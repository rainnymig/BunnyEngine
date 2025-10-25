#version 460

layout(set = 0, binding = 0) uniform sampler2D tex2D;
layout(set = 0, binding = 1) uniform sampler3D tex3D;

layout(push_constant) uniform DisplayParams
{
    float uvZ;  //  the z value when previewing a 3D texture
    uint is3d;
};

layout (location = 0) in vec2 uv;

layout (location = 0) out vec4 outColor;

void main()
{
    if (is3d > 0)
    {
        outColor = texture(tex3D, vec3(uv.x, uv.y, uvZ));
    }
    else
    {
        outColor = texture(tex2D, uv);
    }
    
    //  gamma correction
    float gamma = 2.2;
    outColor.rgb = pow(outColor.rgb, vec3(gamma));
}
