#version 430

layout(binding = 0) uniform sampler2D HDRScene;

layout(location = 0) in vec2 vs_out_texcoord;
layout(location = 0, index = 0) out vec4 FragColor;

void main()
{	
	FragColor = texture2D(HDRScene, vs_out_texcoord);
}