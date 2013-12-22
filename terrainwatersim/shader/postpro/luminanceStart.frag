#version 430

#include "../helper.glsl"

layout(binding = 0) uniform sampler2D HDRScene;

layout(location = 0) in vec2 vs_out_texcoord;
layout(location = 0, index = 0) out float outLuminance;

void main()
{	
    vec3 color = texture(HDRScene, vs_out_texcoord).rgb;
    outLuminance = log(max(CalcLuminance(color), 0.000001f));
}