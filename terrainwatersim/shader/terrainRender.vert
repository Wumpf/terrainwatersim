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
	Out.HeightmapCoord = Out.WorldPos.xz * HeightmapWorldTexelSize;
	Out.WorldPos.xz += GridMinPosition;

	// Todo: Using lower mipmaps could both improve quality and performance!
	Out.WorldPos.y = textureLod(Heightmap, Out.HeightmapCoord, 0).x * HeightmapHeightScale;
}
