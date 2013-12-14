#version 430

#include "landscapeRenderData.glsl"
#include "landscapeRenderFunctions.glsl"

layout(location = 0) in vec2 inPatchRelPosition;
layout(location = 1) in vec2 inPatchWorldPosition;
layout(location = 2) in float inPatchWorldScale;
layout(location = 3) in uint inPatchRotationType;

layout(location = 0) out vec3 outWorldPos;
layout(location = 1) out vec2 outHeightmapCoord;
layout(location = 2) out int outUnderTerrain;

void main()
{	
	outWorldPos.xz = RelPatchPosToWorldPos(inPatchRelPosition, inPatchWorldPosition, inPatchWorldScale, inPatchRotationType, 
			outHeightmapCoord);

	// Todo: Using lower mipmaps could both improve quality and performance!
	vec4 terrainInfo = textureLod(TerrainInfo, outHeightmapCoord, 0);
	outUnderTerrain = terrainInfo.a < 0.1 ? 1 : 0;
	outWorldPos.y = terrainInfo.r + terrainInfo.a;
}