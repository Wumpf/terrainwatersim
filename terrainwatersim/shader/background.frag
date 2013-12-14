#version 430

// NO SCATTERING YET - ONLY GRADIENT

#include "constantbuffers.glsl"
#include "helper.glsl"

// input
layout(location = 0) in vec2 vs_out_texcoord;

layout(binding = 0) uniform samplerCube Skybox;

// output
layout(location = 0, index = 0) out vec4 FragColor;



void main()
{
	vec3 rayDirection = ComputeRayDirection(vs_out_texcoord, InverseViewProjection);
	FragColor = texture(Skybox, rayDirection);
}
