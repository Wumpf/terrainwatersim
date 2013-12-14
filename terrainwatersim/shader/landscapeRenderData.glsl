layout(binding = 5, shared) uniform GlobalLandscapeInfo
{
	vec2 GridMinPosition;
	float HeightmapWorldTexelSize;	// How big a tex is in world size.

	float MaxTesselationFactor;

	// Determines how many triangles per Clip Space unit the shader tries to generate
	float TrianglesPerClipSpaceUnit;
};

layout(binding = 0) uniform sampler2D TerrainInfo;