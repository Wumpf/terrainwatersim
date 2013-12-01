#version 430

#include "constantbuffers.glsl"
#include "landscapeRenderData.glsl"
#include "helper.glsl"

layout(location = 0) in FragInVertexWater In;
layout(location = 0, index = 0) out vec4 FragColor;

// Texture for refraction color
layout(binding = 1) uniform sampler2D RefractionTexture;
layout(binding = 2) uniform samplerCube ReflectionCubemap;
layout(binding = 3) uniform sampler2D FlowMap;
layout(binding = 4) uniform sampler2D Normalmap;
layout(binding = 5) uniform sampler2D Noise;


// Water Rendering constants
layout(binding = 6) uniform WaterRendering
{
	vec3 BigDepthColor;
	vec3 SurfaceColor;
	vec3 ColorExtinctionCoefficient;
	float Opaqueness;

	float SpeedToNormalDistortion;
	float NormalMapRepeat;

	// only these are dynamic (split up?)
	float FlowDistortionStrength;
	float FlowDistortionTimer;
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
	const float RefractionIndex_Water = 1.33f;
	const float RefractionIndex_Air = 1.00029f;
	const float RefractoinAirToWater = RefractionIndex_Air / RefractionIndex_Water;
	const float ReflectionCoefficient = (RefractionIndex_Air - RefractionIndex_Water) * (RefractionIndex_Air - RefractionIndex_Water) / 
									   ((RefractionIndex_Air + RefractionIndex_Water) * (RefractionIndex_Air + RefractionIndex_Water));
							  

	// flow - http://www.slideshare.net/alexvlachos/siggraph-2010-water-flow-in-portal-2
	vec2 flow = textureLod(FlowMap, In.HeightmapCoord, 0.0f).xy;
	float noise = texture(Noise, In.HeightmapCoord*2).x;
	float distortionBlend = abs(fract(FlowDistortionTimer + noise) * 2 - 1);
	vec2 normalmapCoord = In.HeightmapCoord * NormalMapRepeat;
	vec3 normalMapLayer0 = texture(Normalmap, normalmapCoord + flow * (1-distortionBlend) * FlowDistortionStrength).xzy;
	vec3 normalMapLayer1 = texture(Normalmap, normalmapCoord.yx + flow * distortionBlend * FlowDistortionStrength + vec2(0.4f)).xzy;
	vec3 normalMap = mix(normalMapLayer0, normalMapLayer1, distortionBlend);
	vec3 normalMapNormal = normalMap * 2.0f - vec3(1.0f);
		// scale with speed
	normalMapNormal.xz *= SpeedToNormalDistortion * length(flow);

	// Final Normal
	vec3 normal = normalize(ComputeNormal(In.HeightmapCoord) + normalMapNormal);


	// vector to camera and camera distance
	vec3 toCamera = CameraPosition - In.WorldPos;
	float cameraDistance = length(toCamera);
	toCamera /= cameraDistance;



	// Normal dot Light - basic lighting term
	float nDotL = saturate(dot(normal, GlobalDirLightDirection));
	// Normal dot Camera - angle of viewer to water surface
	float nDotV = saturate(dot(normal, toCamera));
	// Schlick-Fresnel approx
	float fresnel = Fresnel(nDotV, ReflectionCoefficient);


	// specular lighting
	// use camera dir reflection instead of light reflection because cam reflection is needed for cubemapReflection
	// Don't worry, the outcome is exactly the same!
	vec3 cameraDirReflection = normalize((2 * nDotV) * normal - toCamera);
  	float specularAmount = pow(max(0.0f, dot(cameraDirReflection, GlobalDirLightDirection)), 8.0f);
  	specularAmount *= fresnel;



	// Needed: amout of water in screenspace on this very pixel
	// This would need raymarching... instead just use this self-made approximation
	vec4 terrainInfo = texture(TerrainInfo, In.HeightmapCoord);
	float waterDepth = terrainInfo.a;
	float waterViewSpaceDepth = waterDepth / saturate(nDotV + 0.1f);



	// Refraction Texture fetch
	// General Problem: Refraction coordinates may lie outside the screen tex (we would need a cubemap)
	// So we just blend over to no refraction at all if necessary
	vec3 refractionVector = Refract(-nDotV, -toCamera, normal, RefractoinAirToWater);
	vec3 underwaterGroudPos = In.WorldPos + refractionVector * waterViewSpaceDepth;
	vec3 underwaterProjective = (ViewProjection * vec4(underwaterGroudPos, 1.0)).xyw;
	vec2 refractiveTexcoord = 0.5f * (underwaterProjective.z + underwaterProjective.xy) / underwaterProjective.z;
	vec3 refractionTexture = textureLod(RefractionTexture, refractiveTexcoord, 0).rgb;
	vec3 projectiveTexture = textureLod(RefractionTexture, In.ProjectiveCoord.xy / In.ProjectiveCoord.z, 0).rgb;

	vec2 refractToScreenMid = abs(refractiveTexcoord * 2.0f - 1.0f);
	float projectiveWeight = saturate(max(refractToScreenMid.x, refractToScreenMid.y));
	projectiveWeight = pow(projectiveWeight, 32);
	refractionTexture = mix(refractionTexture, projectiveTexture, projectiveWeight);


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
	
	// Shore hack against artifacts
	float pixelToGround = In.WorldPos.y - terrainInfo.r;	// this is a bit better than the classic terrainInfo.a approach
	color = mix(refractionTexture, color, saturate(pixelToGround*pixelToGround * 0.25f));


	// Fogging
	color = ApplyFog(color, cameraDistance, toCamera);

	// Color output
	FragColor.rgb = vec3(color);
	FragColor.a = 1.0f;
}