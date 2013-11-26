#version 430

#include "landscapeRenderData.glsl"
#include "landscapeRenderFunctions.glsl"

layout(location = 0) in VertInVertex In;
layout(location = 0) out ContInVertex Out;
layout(location = 2) out int UnderTerrain;

void main()
{	
	Out.WorldPos.xz = RelPatchPosToWorldPos(In, Out.HeightmapCoord);

	// Todo: Using lower mipmaps could both improve quality and performance!
	vec4 terrainInfo = textureLod(Heightmap, Out.HeightmapCoord, 0);
	UnderTerrain = terrainInfo.a < 0.1f ? 1 : 0;
	Out.WorldPos.y = terrainInfo.r + terrainInfo.a;
}
