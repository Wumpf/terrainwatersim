#version 430

#include "terrainRenderData.glsl"

layout(location = 0) in VertInVertex In;
layout(location = 0) out ContInVertex Out;

void main()
{	
	vec2 patchRelPosition = In.PatchRelPosition;
	if(In.PatchRotationType == 4)
		patchRelPosition.xy = 1.0f - patchRelPosition.xy;
	else
	{ 	
		if(In.PatchRotationType % 2 != 0)
			patchRelPosition.y = 1.0f - patchRelPosition.y;
		if(In.PatchRotationType > 1)
			patchRelPosition.xy = patchRelPosition.yx;
	}



    Out.WorldPos.xz = patchRelPosition * In.PatchWorldScale + In.PatchWorldPosition;
	Out.HeightmapCoord = vec2(0.0f);//In.PatchRelPosition * PatchHeightmapTexcoordScale + PatchHeightmapTexcoordPosition;

	// Todo: Using lower mipmaps could both improve quality and performance!
	Out.WorldPos.y = 0.0f; //textureLod(Heightmap, Out.HeightmapCoord, 0).x * HeightmapHeightScale;
}
