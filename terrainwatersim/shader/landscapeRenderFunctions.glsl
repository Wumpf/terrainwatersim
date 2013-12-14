#include "constantbuffers.glsl"
#include "landscapeRenderFunctions.glsl"

// Computes WorldPos from relative patch pos
vec2 RelPatchPosToWorldPos(in vec2 _patchRelPosition, in vec2 patchWorldPosition, in float patchWorldScale, in uint patchRotationType, out vec2 heightmapCoord)
{
	vec2 patchRelPosition = _patchRelPosition;

	// Real rotations are needed - do not break the vertex order, otherwise culling will fail.
	if(patchRotationType % 2 != 0)
	{
		// Rotate 180°
		patchRelPosition = vec2(1.0) - patchRelPosition;
	}
	if(patchRotationType > 1)
	{
		// Rotate 90°
		patchRelPosition -= vec2(0.5);
		patchRelPosition = vec2(patchRelPosition.y, -patchRelPosition.x);
		patchRelPosition += vec2(0.5);
	}

	vec2 worldPos = patchRelPosition * patchWorldScale + patchWorldPosition;
	heightmapCoord = worldPos * HeightmapWorldTexelSize;
	worldPos += GridMinPosition;

	return worldPos;
}


// Estimates the size of a sphere around a given world space edge
float EstimateSphereSizeAroundEdge(vec3 p0, vec3 p1)
{	
	float diameter = distance(p0, p1);
	vec3 edgeMid = (p1 + p0) * 0.5;

	vec3 camXRadius = diameter * vec3(ViewMatrix[0].x, ViewMatrix[1].x, ViewMatrix[2].x);
	vec2 clip0 = (ViewProjection * vec4(edgeMid, 1.0)).xw;
	vec2 clip1 = (ViewProjection * vec4(edgeMid + camXRadius, 1.0)).xw;

	return abs(clip0.x / clip0.y - clip1.x / clip1.y);
};
