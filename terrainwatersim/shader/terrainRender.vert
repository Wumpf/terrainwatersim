#version 430

#include "landscapeRenderData.glsl"
#include "landscapeRenderFunctions.glsl"

layout(location = 0) in VertInVertex In;
layout(location = 0) out ContInVertex Out;

void main()
{	
	Out.WorldPos.xz = RelPatchPosToWorldPos(In, Out.HeightmapCoord);

	// Todo: Using lower mipmaps could both improve quality and performance!
	Out.WorldPos.y = textureLod(TerrainInfo, Out.HeightmapCoord, 0).x;
}
