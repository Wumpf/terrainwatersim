#version 430

#include "terrainRenderData.glsl"

layout(location = 0) in VertInVertex In;
layout(location = 0) out ContInVertex Out;

void main()
{	
    Out.WorldPos.xz = In.PatchRelPosition * PatchWorldScale + PatchWorldPosition;
	Out.HeightmapCoord = vec2(0.0f);//In.PatchRelPosition * PatchHeightmapTexcoordScale + PatchHeightmapTexcoordPosition;

	// Todo: Using lower mipmaps could both improve quality and performance!
	Out.WorldPos.y = 0.0f; //textureLod(Heightmap, Out.HeightmapCoord, 0).x * HeightmapHeightScale;
}
