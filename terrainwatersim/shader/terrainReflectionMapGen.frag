#version 430

#include "constantbuffers.glsl"
#include "landscapeRenderData.glsl"
#include "helper.glsl"

layout(location = 0) in vec2 inHeightmapCoord;

layout(location = 0, index = 0) out vec3 FragColor;

// Terrain textures
layout(binding = 1) uniform sampler2D GrassDiffuse;
layout(binding = 2) uniform sampler2D StoneDiffuse;


layout(binding = 6, shared) uniform TerrainRendering
{
	// (Diffuse) Texture repeat per World unit.
	float TextureRepeat;
	float FresnelReflectionCoefficient;
	float SpecularPower;
};

vec3 ComputeNormal(in vec2 heightmapCoord)
{
	const float worldStep = 1.0;
	vec4 h;
	h[0] = texture(TerrainInfo, heightmapCoord + HeightmapWorldTexelSize*vec2( 0,-worldStep)).r;
	h[1] = texture(TerrainInfo, heightmapCoord + HeightmapWorldTexelSize*vec2( 0, worldStep)).r;
	h[2] = texture(TerrainInfo, heightmapCoord + HeightmapWorldTexelSize*vec2( worldStep, 0)).r;
	h[3] = texture(TerrainInfo, heightmapCoord + HeightmapWorldTexelSize*vec2(-worldStep, 0)).r;
	vec3 vecdz = vec3(0.0, h[1] - h[0], worldStep);
	vec3 vecdx = vec3(worldStep, h[2] - h[3], 0.0);
	return normalize(cross(vecdz, vecdx));
}

const float TextureDepthBlend = 0.2;

vec3 textureBlend(vec3 diff0, float height0, float a0, vec3 diff1, float height1, float a1)
{
    float ma = max(height0 + a0, height1 + a1) - TextureDepthBlend;

    float b0 = max(height0 + a0 - ma, 0);
    float b1 = max(height1 + a1 - ma, 0);

    return (diff0 * b0 + diff1 * b1) / (b0 + b1);
}
vec4 textureBlend(vec4 diff0, float height0, float a0, vec4 diff1, float height1, float a1)
{
    float ma = max(height0 + a0, height1 + a1) - TextureDepthBlend;

    float b0 = max(height0 + a0 - ma, 0);
    float b1 = max(height1 + a1 - ma, 0);

    return (diff0 * b0 + diff1 * b1) / (b0 + b1);
}

void main()
{
	vec2 terrainTexcoord = inHeightmapCoord;
	
	// Normal
	vec3 normal = ComputeNormal(terrainTexcoord);

	// texturingss
	vec2 texcoord = TextureRepeat * terrainTexcoord / HeightmapWorldTexelSize;
	float textureSlopeFactor = normal.y;
	
	vec4 grass_diffuse_spec = texture(GrassDiffuse, texcoord);
	vec4 stone_diffuse_spec = texture(StoneDiffuse, texcoord);

	// cool height based blending
	vec4 diffuseColor_spec = mix(stone_diffuse_spec, grass_diffuse_spec, textureSlopeFactor* textureSlopeFactor);

	// Lighting
	float nDotL = dot(normal, GlobalDirLightDirection);
	// Diffuse Lighting
	float lighting = clamp(nDotL, 0, 1);
	// good old backlighting hack instead of plain ambient!
	vec3 backLightDir = GlobalDirLightDirection;
	backLightDir.xz = -backLightDir.xz;
	float ambientLightAmount = clamp(dot(normal, backLightDir) - 0.2, 0, 1);


	// Color compositing.
	FragColor = diffuseColor_spec.rgb * (GlobalDirLightColor * lighting + GlobalAmbient * ambientLightAmount);
}