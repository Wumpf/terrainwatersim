#version 430

#include "terrainRenderData.glsl"

layout(location = 0) in VertInVertex In;
layout(location = 0) out ContInVertex Out;

void main()
{	
	vec2 patchRelPosition = In.PatchRelPosition;

	// Real rotations are needed - do not break the vertex order, otherwise culling will fail.
	if(In.PatchRotationType % 2 != 0)
	{
		// Rotate 180°
		patchRelPosition = vec2(1.0) - patchRelPosition;
	}
	if(In.PatchRotationType > 1)
	{
		// Rotate 90°
		patchRelPosition -= vec2(0.5);
		patchRelPosition = vec2(patchRelPosition.y, -patchRelPosition.x);
		patchRelPosition += vec2(0.5);
	}

    Out.WorldPos.xz = patchRelPosition * In.PatchWorldScale + In.PatchWorldPosition;
	Out.HeightmapCoord = Out.WorldPos.xz * HeightmapWorldTexelSize;
	Out.WorldPos.xz += GridMinPosition;

	// Todo: Using lower mipmaps could both improve quality and performance!
	Out.WorldPos.y = textureLod(Heightmap, Out.HeightmapCoord, 0).x * HeightmapHeightScale;
}
