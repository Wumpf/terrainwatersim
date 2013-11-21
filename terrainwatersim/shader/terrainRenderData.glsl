layout(binding = 5, shared) uniform GlobalTerrainInfo
{
	float HeightmapHeightScale;
	float HeightmapTexelSize;
	float HeightmapTexelSizeWorld_doubled;	// size of a texel in worldcoordinates doubled
	
	float MaxTesselationFactor;

	// Determines how many triangles per Clip Space unit the shader tries to generate
	float TrianglesPerClipSpaceUnit;

	/*float DetailHeightScale;
	float DetailHeightmapTexcoordFactor;
	float DetailHeightmapTexelSize;	
	float DetailHeightmapTexelSizeWorld_doubled;	// size of a texel in worldcoordinates doubled

	float TextureRepeat; */
};

//layout(binding = 0) uniform sampler2D Heightmap;

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