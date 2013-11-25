#version 430

#include "constantbuffers.glsl"
#include "landscapeRenderData.glsl"

layout(location = 0) in FragInVertex In;
layout(location = 0, index = 0) out vec4 FragColor;

void main()
{
	FragColor = vec4(0,0,1,1);
}