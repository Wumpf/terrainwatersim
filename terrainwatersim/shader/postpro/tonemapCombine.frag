#version 430

#include "../helper.glsl"
#include "postproCommon.glsl"

layout(binding = 0) uniform sampler2D HDRScene;
layout(binding = 1) uniform sampler2D LinearLuminance;

layout(location = 0) in vec2 vs_out_texcoord;
layout(location = 0, index = 0) out vec4 FragColor;

// Applies the filmic curve from John Hable's presentation
vec3 ToneMapFilmicALU(vec3 color)
{
    color = max(vec3(0.0), color - vec3(0.004));
    color = (color * (6.2 * color + vec3(0.5))) / (color * (6.2 * color + vec3(1.7)) + vec3(0.06));

    // result has 1/2.2 baked in
    return pow(color, vec3(2.2));
}

// Applies exposure and tone mapping to the specific color, and applies
// the threshold to the exposure value.
vec3 ToneMap(vec3 color)
{
	float avgLuminance = texture(LinearLuminance, vec2(0.5)).r;
    color *= TonemapExposure / avgLuminance;
	color = ToneMapFilmicALU(color);
    return color;
}

void main()
{	
	vec3 hdrColor = texture(HDRScene, vs_out_texcoord).rgb;
	FragColor.rgb = ToneMap(hdrColor);
	FragColor.a = 1.0f;
}