#version 430

#include "constantbuffers.glsl"
#include "landscapeRenderData.glsl"

layout(location = 0) in FragInVertex In;
layout(location = 0, index = 0) out vec4 FragColor;

vec3 ComputeNormal(in vec2 heightmapCoord)
{
	const float worldStep = 1.0f;
	vec4 terrainInfo;
	vec4 h;
	terrainInfo = texture(Heightmap, heightmapCoord + HeightmapWorldTexelSize*vec2( 0,-worldStep));
	h[0] = terrainInfo.a + terrainInfo.r;
	terrainInfo = texture(Heightmap, heightmapCoord + HeightmapWorldTexelSize*vec2( 0, worldStep));
	h[1] = terrainInfo.a + terrainInfo.r;
	terrainInfo = texture(Heightmap, heightmapCoord + HeightmapWorldTexelSize*vec2( worldStep, 0));
	h[2] = terrainInfo.a + terrainInfo.r;
	terrainInfo = texture(Heightmap, heightmapCoord + HeightmapWorldTexelSize*vec2(-worldStep, 0));
	h[3] = terrainInfo.a + terrainInfo.r;
	vec3 vecdz = vec3(0.0f, h[1] - h[0], worldStep);
	vec3 vecdx = vec3(worldStep, h[2] - h[3], 0.0f);
	return normalize(cross(vecdz, vecdx));
}

void main()
{
	vec3 normal = ComputeNormal(In.HeightmapCoord);
	float nDotL = dot(normal, GlobalDirLightDirection) + 0.2f;

	FragColor = vec4(0,0,1,1) * nDotL;
}