layout(binding = 5, shared) uniform GlobalTerrainInfo
{
	vec2 GridMinPosition;
	float HeightmapHeightScale;
	float HeightmapWorldTexelSize;	// How big a tex is in world size.

	float MaxTesselationFactor;

	// Determines how many triangles per Clip Space unit the shader tries to generate
	float TrianglesPerClipSpaceUnit;

	// 1.0 / HeightmapSize
	float HeightmapTexelSize; 
};

layout(binding = 0) uniform sampler2D Heightmap;

// Shader out/in
struct VertInVertex
{
	vec2 PatchRelPosition;
	vec2 PatchWorldPosition;
	float PatchWorldScale;
	uint PatchRotationType; // 0: Y-, 1: Y+, 2: X-, 3: X+, 4: Special corner
};
struct ContInVertex
{
	vec3 WorldPos;
	vec2 HeightmapCoord;
};
struct EvalInVertex
{
	vec2 WorldPos2D;
	vec2 HeightmapCoord;
};
struct FragInVertex
{
	vec3 WorldPos;
	vec2 HeightmapCoord;
};