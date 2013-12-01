#version 430

#include "constantbuffers.glsl"
#include "landscapeRenderData.glsl"
#include "helper.glsl"

layout(location = 0) in FragInVertexWater In;
layout(location = 0, index = 0) out vec4 FragColor;

// Texture for refraction color
layout(binding = 1) uniform sampler2D RefractionTexture;
layout(binding = 2) uniform samplerCube ReflectionCubemap;


// Water Rendering constants
layout(binding = 6) uniform WaterRendering
{
	vec3 BigDepthColor;
	vec3 SurfaceColor;
	vec3 ColorExtinctionCoefficient;
	float Opaqueness;
};

vec3 ComputeNormal(in vec2 heightmapCoord)
{
	const float worldStep = 1.0f;
	vec4 terrainInfo;
	vec4 h;
	terrainInfo = texture(TerrainInfo, heightmapCoord + HeightmapWorldTexelSize*vec2( 0,-worldStep));
	h[0] = terrainInfo.a + terrainInfo.r;
	terrainInfo = texture(TerrainInfo, heightmapCoord + HeightmapWorldTexelSize*vec2( 0, worldStep));
	h[1] = terrainInfo.a + terrainInfo.r;
	terrainInfo = texture(TerrainInfo, heightmapCoord + HeightmapWorldTexelSize*vec2( worldStep, 0));
	h[2] = terrainInfo.a + terrainInfo.r;
	terrainInfo = texture(TerrainInfo, heightmapCoord + HeightmapWorldTexelSize*vec2(-worldStep, 0));
	h[3] = terrainInfo.a + terrainInfo.r;
	vec3 vecdz = vec3(0.0f, h[1] - h[0], worldStep);
	vec3 vecdx = vec3(worldStep, h[2] - h[3], 0.0f);
	return normalize(cross(vecdz, vecdx));
}

void main()
{
	// Surface Normal
	vec3 normal = ComputeNormal(In.HeightmapCoord);

	// vector to camera and camera distance
	vec3 toCamera = CameraPosition - In.WorldPos;
	float cameraDistance = length(toCamera);
	toCamera /= cameraDistance;



	// Normal dot Light - basic lighting term
	float nDotL = saturate(dot(normal, GlobalDirLightDirection));
	// Normal dot Camera - angle of viewer to water surface
	float nDotV = saturate(dot(normal, toCamera));
	// Schlick-Fresnel approx
	float fresnel = Fresnel(nDotV, 0.2f);


	// specular lighting
	// use camera dir reflection instead of light reflection because cam reflection is needed for cubemapReflection
	// Don't worry, the outcome is exactly the same!
	vec3 cameraDirReflection = normalize((2 * nDotV) * normal - toCamera);
  	float specularAmount = pow(max(0.0f, dot(cameraDirReflection, GlobalDirLightDirection)), 8.0f);
  	specularAmount *= fresnel;



	// Needed: amout of water in screenspace on this very pixel
	// This would need raymarching... instead just use this self-made approximation
	float waterDepth = texture(TerrainInfo, In.HeightmapCoord).a;
	float waterViewSpaceDepth = waterDepth / nDotV;



	// Refraction Texture fetch
	const float WaterRefractionIndex = 1.0f / 1.33333333333f;
	vec3 refractionVector = Refract(-toCamera, normal, WaterRefractionIndex);
	vec3 underwaterGroudPos = In.WorldPos + refractionVector * waterViewSpaceDepth * 0.5f;
	vec4 underwaterProjective = (ViewProjection * vec4(underwaterGroudPos, 1.0));
		// This is ugly, but nobody won't notice, trust me ;)
	if(any(lessThan(underwaterProjective.xy, vec2(-underwaterProjective.w))) || any(greaterThan(underwaterProjective.xy, vec2(underwaterProjective.w))))
		underwaterProjective = In.ProjectiveCoord;
	else
		underwaterProjective.xy = 0.5f * (underwaterProjective.w + underwaterProjective.xy);
	vec3 refractionTexture = textureProj(RefractionTexture, underwaterProjective).rgb;


	// Water color
	// This otherwise quite convincing reference assumes linear extinction, I'll go with exponential- http://www.gamedev.net/page/reference/index.html/_/technical/graphics-programming-and-theory/rendering-water-as-a-post-process-effect-r2642
	// All non-refractive parts (water self color) are lit with nDotL
	vec3 colorExtinction = clamp(exp(-waterViewSpaceDepth * ColorExtinctionCoefficient), 0, 1);
	vec3 waterColor = mix(refractionTexture, SurfaceColor * GlobalDirLightColor * nDotL, saturate(waterViewSpaceDepth * Opaqueness));
	vec3 refractionColor = mix(BigDepthColor * nDotL, waterColor, colorExtinction);

	// Reflection
	vec3 reflectionColor = texture(ReflectionCubemap, cameraDirReflection).rgb;


	// Combine Refraction & Reflection & Specular
	vec3 color = mix(refractionColor, reflectionColor, saturate(fresnel)) + GlobalDirLightColor * specularAmount;



	// Fogging
	color = ApplyFog(color, cameraDistance, toCamera);

	// Color output
	FragColor.rgb = color; //vec3(waterViewSpaceDepth*0.01f);
	FragColor.a = 1.0f;
}