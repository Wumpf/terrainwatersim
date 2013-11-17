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

layout(binding = 6, shared) uniform TerrainPatchInfo
{
	vec2 PatchWorldPosition;					// 2D Position in world
	vec2 PatchHeightmapTexcoordPosition;		// 2D Position on heightmap

	float PatchWorldScale;					// Scale of the Patch
	float PatchHeightmapTexcoordScale;		// Size of the patch on the heightmap.
};

//layout(binding = 0) uniform sampler2D Heightmap;

// Shader out/in
struct VertInVertex
{
	vec2 PatchRelPosition;
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