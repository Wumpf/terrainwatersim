#version 430

#include "postproCommon.glsl"

layout(binding = 0) uniform sampler2D LogLuminance;
layout(binding = 1) uniform sampler2D LastLinLuminance;

layout(location = 0) in vec2 vs_out_texcoord;
layout(location = 0, index = 0) out float outLuminance;

void main()
{	
    float lastLum = texture(LastLinLuminance, vec2(0.5)).r;
    float currentLum = exp(textureLod(LogLuminance, vec2(0.5, 0.5), 99.0).r);

    // Adapt the luminance using Pattanaik's technique
    outLuminance = lastLum + (currentLum - lastLum) * LuminanceInterpolator;
}