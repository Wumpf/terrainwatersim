#version 430

#include "constantbuffers.glsl"
#include "terrainRenderData.glsl"

layout(location = 0) in FragInVertex In;
layout(location = 0, index = 0) out vec4 FragColor;

vec3 ComputeNormal(in vec2 heightmapCoord)
{
	const float worldStep = 1.0f;
	vec4 h;
	h[0] = texture(Heightmap, heightmapCoord + HeightmapWorldTexelSize*vec2( 0,-worldStep)).r;
	h[1] = texture(Heightmap, heightmapCoord + HeightmapWorldTexelSize*vec2( 0, worldStep)).r;
	h[2] = texture(Heightmap, heightmapCoord + HeightmapWorldTexelSize*vec2( worldStep, 0)).r;
	h[3] = texture(Heightmap, heightmapCoord + HeightmapWorldTexelSize*vec2(-worldStep, 0)).r;
	h *= HeightmapHeightScale;
	vec3 vecdz = vec3(0.0f, h[1] - h[0], worldStep);
	vec3 vecdx = vec3(worldStep, h[2] - h[3], 0.0f);
	return normalize(cross(vecdz, vecdx));
}

vec3 textureBlend(vec3 diff0, float height0, float a0, vec3 diff1, float height1, float a1)
{
    float depth = 0.2f;
    float ma = max(height0 + a0, height1 + a1) - depth;

    float b0 = max(height0 + a0 - ma, 0);
    float b1 = max(height1 + a1 - ma, 0);

    return (diff0 * b0 + diff1 * b1) / (b0 + b1);
}
vec4 textureBlend(vec4 diff0, float height0, float a0, vec4 diff1, float height1, float a1)
{
    float depth = 0.2f;
    float ma = max(height0 + a0, height1 + a1) - depth;

    float b0 = max(height0 + a0 - ma, 0);
    float b1 = max(height1 + a1 - ma, 0);

    return (diff0 * b0 + diff1 * b1) / (b0 + b1);
}

void main()
{
	// Normal
	vec3 heightmapNormal = ComputeNormal(In.HeightmapCoord);

	// texturing
	vec2 texcoord = In.WorldPos.xz*TextureRepeat;
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
	vec3 normal = normalize(textureNormal + heightmapNormal*2);

	// Lighting
	float nDotL = dot(normal, GlobalDirLightDirection);

	// specular lighting
	vec3 refl = normalize((2 * nDotL) * normal - GlobalDirLightDirection);
	vec3 viewDir = normalize(CameraPosition - In.WorldPos);
  	float specularAmount = pow(max(0.0f, dot(refl, viewDir)), 4.0f) * diffuseColor_spec.a;

	// Schlick-Fresnel approx
	vec3 halfVector_viewspace = normalize(GlobalDirLightDirection + viewDir);
	float base = 1.0 - dot(GlobalDirLightDirection, halfVector_viewspace);
	float exponential = pow(base, 5.0f);
	float fresnel = exponential + 0.2f * (1.0f - exponential);
	specularAmount *= fresnel;
	

	// Diffuse Lighting
	float lighting = clamp(nDotL, 0, 1);

	// good old backlighting hack instead of plain ambient!
	vec3 backLightDir = GlobalDirLightDirection;
	backLightDir.xz = -backLightDir.xz;
	float ambientLightAmount = clamp(dot(normal, backLightDir) - 0.2f, 0, 1);


	// Color compositing.
	FragColor.xyz = diffuseColor_spec.rgb * (GlobalDirLightColor * lighting + GlobalAmbient * ambientLightAmount) + specularAmount * GlobalDirLightColor;

	// Normal debugging:
	//FragColor.xyz = abs(vec3(normal.x, normal.y,normal.z));
	FragColor.a = 1.0f;
}
