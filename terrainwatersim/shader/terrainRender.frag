#version 430

#include "constantbuffers.glsl"
#include "landscapeRenderData.glsl"
#include "helper.glsl"

layout(location = 0) in FragInVertexTerrain In;
layout(location = 0, index = 0) out vec4 FragColor;

// Terrain textures
layout(binding = 1) uniform sampler2D GrassDiffuse;
layout(binding = 2) uniform sampler2D StoneDiffuse;
layout(binding = 3) uniform sampler2D GrassNormal;
layout(binding = 4) uniform sampler2D StoneNormal;


layout(binding = 6, shared) uniform TerrainRendering
{
	// (Diffuse) Texture repeat per World unit.
	float TextureRepeat;
	float FresnelReflectionCoefficient;
	float SpecularPower;
};

vec3 ComputeNormal(in vec2 heightmapCoord)
{
	const float worldStep = 1.0f;
	vec4 h;
	h[0] = texture(TerrainInfo, heightmapCoord + HeightmapWorldTexelSize*vec2( 0,-worldStep)).r;
	h[1] = texture(TerrainInfo, heightmapCoord + HeightmapWorldTexelSize*vec2( 0, worldStep)).r;
	h[2] = texture(TerrainInfo, heightmapCoord + HeightmapWorldTexelSize*vec2( worldStep, 0)).r;
	h[3] = texture(TerrainInfo, heightmapCoord + HeightmapWorldTexelSize*vec2(-worldStep, 0)).r;
	vec3 vecdz = vec3(0.0f, h[1] - h[0], worldStep);
	vec3 vecdx = vec3(worldStep, h[2] - h[3], 0.0f);
	return normalize(cross(vecdz, vecdx));
}

const float TextureDepthBlend = 0.2f;

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
	// Normal
	vec3 heightmapNormal = ComputeNormal(In.HeightmapCoord);

	// texturing
	vec2 texcoord = In.WorldPos.xz*TextureRepeat*1.5f;
	float textureSlopeFactor = heightmapNormal.y;
	
	vec4 grass_diffuse_spec = texture(GrassDiffuse, texcoord);
	vec4 stone_diffuse_spec = texture(StoneDiffuse, texcoord);

	vec4 grass_normal_height = texture(GrassNormal, texcoord);
	vec4 stone_normal_height = texture(StoneNormal, texcoord);

	// cool height based blending
	vec4 diffuseColor_spec = textureBlend(grass_diffuse_spec, grass_normal_height.a, textureSlopeFactor, 
										 stone_diffuse_spec, stone_normal_height.a, 1.0f - textureSlopeFactor);
	vec3 textureNormal = textureBlend(grass_normal_height.rgb, grass_normal_height.a, textureSlopeFactor, 
									 stone_normal_height.rgb, stone_normal_height.a, 1.0f - textureSlopeFactor);

	// old simple mix:
	//diffuseColor_spec = mix(grass_diffuse_spec, stone_diffuse_spec, 1.0f - textureSlopeFactor);
	//textureNormal = mix(stone_normal_height.xyz, stone_normal_height.xyz, 1.0f - textureSlopeFactor);

	// Final normal
	textureNormal = textureNormal.xzy * 2.0f - 1.0f;
	textureNormal = normalize(textureNormal);
	vec3 normal = normalize(textureNormal + heightmapNormal);

	// Lighting
	float nDotL = dot(normal, GlobalDirLightDirection);

	// specular lighting
	vec3 refl = normalize((2 * nDotL) * normal - GlobalDirLightDirection);
	vec3 toCamera = CameraPosition - In.WorldPos;
	float cameraDistance = length(toCamera);
	toCamera /= cameraDistance;
  	float specularAmount = pow(max(0.0f, dot(refl, toCamera)), SpecularPower) * diffuseColor_spec.a;

	// Schlick-Fresnel approx
	float fresnel = Fresnel(dot(normal, toCamera), FresnelReflectionCoefficient);
	specularAmount *= fresnel;

	// Diffuse Lighting
	float lighting = clamp(nDotL, 0, 1);

	// good old backlighting hack instead of plain ambient!
	vec3 backLightDir = GlobalDirLightDirection;
	backLightDir.xz = -backLightDir.xz;
	float ambientLightAmount = clamp(dot(normal, backLightDir) - 0.2f, 0, 1);


	// Color compositing.
	FragColor.xyz = diffuseColor_spec.rgb * (GlobalDirLightColor * lighting + GlobalAmbient * ambientLightAmount) + specularAmount * GlobalDirLightColor;

	// clever fog http://www.iquilezles.org/www/articles/fog/fog.htm
	FragColor.xyz = ApplyFog(FragColor.xyz, cameraDistance, toCamera);
	FragColor.a = 1.0f;
}