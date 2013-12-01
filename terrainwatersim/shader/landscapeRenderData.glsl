layout(binding = 5, shared) uniform GlobalLandscapeInfo
{
	vec2 GridMinPosition;
	float HeightmapWorldTexelSize;	// How big a tex is in world size.

	float MaxTesselationFactor;

	// Determines how many triangles per Clip Space unit the shader tries to generate
	float TrianglesPerClipSpaceUnit;

	// (Diffuse) Texture repeat per World unit.
	float TextureRepeat;
};

layout(binding = 0) uniform sampler2D TerrainInfo;

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


struct FragInVertexTerrain
{
	vec3 WorldPos;
	vec2 HeightmapCoord;
};
struct FragInVertexWater
{
	vec3 WorldPos;
	vec4 ProjectiveCoord;
	vec2 HeightmapCoord;
};