#version 430

// input
layout(location = 0) in vec2 inTexcoord;
layout(binding = 0) uniform sampler2D LowerMip;

// output
layout(location = 0, index = 0) out float MaxValue;


void main()
{
	vec4 heightValues = textureGather(LowerMip, inTexcoord, int(0));
	MaxValue = max(max(max(heightValues.x, heightValues.y), heightValues.z), heightValues.w);
}