#include "constantbuffers.glsl"
#include "landscapeRenderFunctions.glsl"

// Computes WorldPos from relative patch pos
vec2 RelPatchPosToWorldPos(in VertInVertex vert, out vec2 heightmapCoord)
{
	vec2 patchRelPosition = vert.PatchRelPosition;

	// Real rotations are needed - do not break the vertex order, otherwise culling will fail.
	if(vert.PatchRotationType % 2 != 0)
	{
		// Rotate 180°
		patchRelPosition = vec2(1.0) - patchRelPosition;
	}
	if(vert.PatchRotationType > 1)
	{
		// Rotate 90°
		patchRelPosition -= vec2(0.5);
		patchRelPosition = vec2(patchRelPosition.y, -patchRelPosition.x);
		patchRelPosition += vec2(0.5);
	}

	vec2 worldPos = patchRelPosition * vert.PatchWorldScale + vert.PatchWorldPosition;
	heightmapCoord = worldPos * HeightmapWorldTexelSize;
	worldPos += GridMinPosition;

	return worldPos;
}


// Estimates the size of a sphere around a given world space edge
float EstimateSphereSizeAroundEdge(vec3 p0, vec3 p1)
{	
	float diameter = distance(p0, p1);
	vec3 edgeMid = (p1 + p0) * 0.5f;

	vec3 camXRadius = diameter * vec3(ViewMatrix[0].x, ViewMatrix[1].x, ViewMatrix[2].x);
	vec2 clip0 = (ViewProjection * vec4(edgeMid, 1.0f)).xw;
	vec2 clip1 = (ViewProjection * vec4(edgeMid + camXRadius, 1.0f)).xw;

	return abs(clip0.x / clip0.y - clip1.x / clip1.y);
};
