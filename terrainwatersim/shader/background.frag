#version 430

#include "constantbuffers.glsl"
#include "helper.glsl"

//uniform vec3 LightDirection;
const vec3 LightDirection = vec3(0, -0.333, 0.333);

// input
in vec2 vs_out_texcoord;

// output
layout(location = 0, index = 0) out vec4 fragColor;

// ------------------------------------------------
// SKY
// ------------------------------------------------
const vec3 AdditonalSunColor = vec3(1.0, 0.98, 0.8)/3;
const vec3 LowerHorizonColour = vec3(0.815, 1.141, 1.54)/2;
const vec3 UpperHorizonColour = vec3(0.986, 1.689, 2.845)/2;
const vec3 UpperSkyColour = vec3(0.16, 0.27, 0.43)*0.8;
const vec3 GroundColour = vec3(0.31, 0.41, 0.5)*0.8;
const float LowerHorizonHeight = -0.4;
const float UpperHorizonHeight = -0.1;
const float SunAttenuation = 2;
vec3 computeSkyColor(in vec3 ray)
{
	vec3 color;

	// background
	float heightValue = ray.y;	// mirror..
	if(heightValue < LowerHorizonHeight)
		color = mix(GroundColour, LowerHorizonColour, (heightValue+1) / (LowerHorizonHeight+1));
	else if(heightValue < UpperHorizonHeight)
		color = mix(LowerHorizonColour, UpperHorizonColour, (heightValue-LowerHorizonHeight) / (UpperHorizonHeight - LowerHorizonHeight));
	else
		color = mix(UpperHorizonColour, UpperSkyColour, (heightValue-UpperHorizonHeight) / (1.0-UpperHorizonHeight));
	
	// Sun
	float angle = max(0, dot(ray, LightDirection));
	color += (pow(angle, SunAttenuation) + pow(angle, 10000)*10) * AdditonalSunColor;

	return color;
}

// ------------------------------------------------
// MAIN
// ------------------------------------------------
void main()
{
	vec3 rayDirection = ComputeRayDirection(vs_out_texcoord, InverseViewProjection);

	// Color
	fragColor.a = 0.0f;
	fragColor.rgb = computeSkyColor(rayDirection);
}
