#version 430

#include "constantbuffers.glsl"
#include "landscapeRenderData.glsl"
#include "helper.glsl"

layout(location = 0) in vec3 inWorldPos;
layout(location = 1) in vec2 inHeightmapCoord;
layout(location = 2) in vec4 inProjectiveCoord;

layout(location = 0, index = 0) out vec4 FragColor;

// Texture for refraction color
layout(binding = 1) uniform sampler2D SceneTexture;
layout(binding = 2) uniform samplerCube ReflectionCubemap;
layout(binding = 3) uniform sampler2D FlowMap;
layout(binding = 4) uniform sampler2D Normalmap;
layout(binding = 5) uniform sampler2D Noise;
layout(binding = 6) uniform sampler2D Foam;
layout(binding = 7) uniform sampler2D TerrainInfoFiltered;

layout(binding = 8) uniform sampler2D SceneDepthTexture;


// Water Rendering constants
layout(binding = 6) uniform WaterRendering
{
	vec3 BigDepthColor;
	vec3 SurfaceColor;
	vec3 ColorExtinctionCoefficient;
	float Opaqueness;

	float SpeedToNormalDistortion;
	float NormalMapRepeat;
	float FlowDistortionStrength;

	// only these are dynamic (split up?)
	float FlowDistortionTimer;
};

const float RefractionIndex_Water = 1.33;
const float RefractionIndex_Air = 1.00029;
const float RefractionAirToWater = RefractionIndex_Air / RefractionIndex_Water;
const float ReflectionCoefficient = (RefractionIndex_Air - RefractionIndex_Water) * (RefractionIndex_Air - RefractionIndex_Water) / 
								   ((RefractionIndex_Air + RefractionIndex_Water) * (RefractionIndex_Air + RefractionIndex_Water));


vec3 ComputeHeightmapNormal(in vec2 heightmapCoord)
{
	const float worldStep = 1.0;
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
	vec3 vecdz = vec3(0.0, h[1] - h[0], worldStep);
	vec3 vecdx = vec3(worldStep, h[2] - h[3], 0.0);
	return normalize(cross(vecdz, vecdx));
}

vec3 ComputeRefractionColor(float nDotV, float nDotL, vec3 toCamera, vec3 normal, float waterDepth, vec2 projectiveCoord, out vec3 refractionTexture) 
{
	vec3 refractionVector = Refract(nDotV, toCamera, normal, RefractionAirToWater);

	// basically same hack as with the viewspace water depth hack
	float waterRefractionDepth = waterDepth / saturate(-dot(normal, refractionVector) + 0.1);

	// This was a promising attempt to mimic correct refraction
	// Basic problems:
	// * under water pos sometimes not under water
	// * under water pos sometimes not in screen. This approach tried to blend over - ugly happens far to often!

/*	vec3 underwaterGroudPos = In.WorldPos + refractionVector * waterRefractionDepth;
	vec3 underwaterProjective = (ViewProjection * vec4(underwaterGroudPos, 1.0)).xyw;
	vec2 refractiveTexcoord = 0.5f * (underwaterProjective.z + underwaterProjective.xy) / underwaterProjective.z;
	SceneTexture = textureLod(SceneTexture, refractiveTexcoord, 0).rgb;
	// General Problem: Refraction coordinates may lie outside the screen tex (we would need a cubemap)
	// So we just blend over to no refraction at all if necessary
	// Projective Texture has also a bit distortion, so the effect won't  be that bad
	vec3 projectiveTexture = textureLod(SceneTexture, saturate(projectiveCoord.xy / projectiveCoord.z + normal.xz*0.1), 0).rgb;

	vec2 refractToScreenMid = abs(refractiveTexcoord * 2.0 - 1.0);
	float projectiveWeight = saturate(max(refractToScreenMid.x, refractToScreenMid.y));
	projectiveWeight = pow(projectiveWeight, 16);
	SceneTexture = mix(SceneTexture, projectiveTexture, projectiveWeight);
*/

	refractionTexture = textureLod(SceneTexture, saturate(projectiveCoord + normal.xz* waterRefractionDepth * 0.05), 0).rgb;


	// Water color
	// This otherwise quite convincing reference assumes linear extinction, I'll go with exponential- http://www.gamedev.net/page/reference/index.html/_/technical/graphics-programming-and-theory/rendering-water-as-a-post-process-effect-r2642
	// This leaves the questions: What are convining Extinction coefficients?
	
	// For that I solved the equations exp(-4.5 * R) = exp(-75 * G) = exp(-300 * B) = 0.001 (depths are from link above)
	// .. which is.. convincing pseudo physical! :)
	//const vec3 ColorExtinctionCoefficient_ = vec3(1.53506f, 0.0921034f, 0.0230259f);
	// But tweakable is still better... 

	// All non-refractive parts (water self color) are lit with nDotL

	vec3 colorExtinction = clamp(exp(-waterRefractionDepth * ColorExtinctionCoefficient), 0, 1);
	vec3 normalLightingColor = GlobalDirLightColor * nDotL + GlobalAmbient;
	vec3 waterColor = mix(refractionTexture, SurfaceColor * normalLightingColor, saturate(waterRefractionDepth * Opaqueness));

	return mix(BigDepthColor, waterColor, colorExtinction);
}


#define XNEG 1
#define YNEG 2
//#define ZNEG 4
bool MaxMapBasedDepthBufferRaymarching(in vec3 texEntry, in vec3 rayDirection_TextureSpace, out vec2 intersectionTexcoord, const uint config)
{
	int numIterations = 0;
	const int maxIterations = 20;

	// Currently examined miplevel, do not start at lowest level
	int currentMipLevel = textureQueryLevels(SceneDepthTexture)-3; // Start with 16xX texture.
	vec2 currentLevelResolution = textureSize(SceneDepthTexture, currentMipLevel);//8;

	vec3 rayDirection_TexelSpace = rayDirection_TextureSpace;
	rayDirection_TexelSpace.xy *= currentLevelResolution;
	texEntry.xy *= currentLevelResolution;

	while (all(lessThan(texEntry.xy, currentLevelResolution) && greaterThan(texEntry.xy, vec2(0.0))))
	{
		// Anti-Endlessloop code.
		//++numIterations;
		//if (numIterations > maxIterations)
		// 	break;

		// Compute tex exit
		vec2 tXY;// = (float2(1.0f, 1.0f) - frac(texEntry.xz)) / rayDirection_TexelSpace.xz;
		tXY.x = ((config & XNEG) > 0) ? fract(texEntry.x) : (1.0 - fract(texEntry.x));
		tXY.y = ((config & YNEG) > 0) ? fract(texEntry.y) : (1.0 - fract(texEntry.y));
		tXY /= abs(rayDirection_TexelSpace.xy);

		vec3 texExit;
		if(tXY.x < tXY.y)
		{
		  	texExit.yz = texEntry.yz + rayDirection_TexelSpace.yz * (tXY.x);
		  	// The line above should do exactly that even for the missing component, but numeric instability kicks in.
		  	// The problem with negative stepping is that we have to express that we are staying closely *behind* the border
		  	// Because of this there a epsilon subtracted.
		  	texExit.x = ((config & XNEG) > 0) ? ceil(texEntry.x) - 1.0001 : (floor(texEntry.x) + 1.0);
		}
		else
		{
			texExit.xz = texEntry.xz + rayDirection_TexelSpace.xz * tXY.y;
			// Same here...
			texExit.y = ((config & YNEG) > 0) ? ceil(texEntry.y) - 1.0001 : (floor(texEntry.y) + 1.0);
		}

		// Sample Height - some configurations need mirrored coordinates.
		vec2 samplingPosition = texEntry.xy;
		float heightValue = texelFetch(SceneDepthTexture, ivec2(samplingPosition), currentMipLevel).r;

		// Hit?
		if (heightValue < texExit.z)
		{
			// "Descending" on depth -> new hit!
			vec3 intersection = texExit - max((texExit.z - heightValue) / rayDirection_TexelSpace.z, 0) * rayDirection_TexelSpace;
			//texExit.z = heightValue;

			if (currentMipLevel == 0)
			{
				intersectionTexcoord = texEntry.xy / currentLevelResolution;
				return heightValue != 1.0;
			}
			else
			{
				// Todo: Currently only going down, but could also go up in certain situations! This COULD help performance, could also make it worse

				// Problem: Depth buffer is not square! Need to check
				--currentMipLevel;
				intersection.xy /= currentLevelResolution;
				
				currentLevelResolution = textureSize(SceneDepthTexture, currentMipLevel);
				rayDirection_TexelSpace.xy = rayDirection_TextureSpace.xy * currentLevelResolution;
				
				texEntry = intersection;
				texEntry.xy = texEntry.xy * currentLevelResolution;
			}
		}
		else
			texEntry = texExit;
	}

	return false;
}

void main()
{
	// flow - http://www.slideshare.net/alexvlachos/siggraph-2010-water-flow-in-portal-2
	vec2 flow = textureLod(FlowMap, inHeightmapCoord, 0.0).xy;
	float noise = texture(Noise, inHeightmapCoord*6).x;

	float distortTimer = FlowDistortionTimer + noise;
	float distortionFactor0 = fract(distortTimer);
	float distortionFactor1 = fract(distortTimer + 0.5);
	float distortionBlend = abs(distortionFactor0 * 2 - 1);

	vec2 normalmapCoord = inHeightmapCoord * NormalMapRepeat;
	vec2 flowDistortion = flow * FlowDistortionStrength;
	vec2 normalmapCoord0 = normalmapCoord + distortionFactor0 * flowDistortion;
	vec2 normalmapCoord1 = normalmapCoord + distortionFactor1 * flowDistortion + vec2(0.3);

	vec3 normalMapLayer0 = texture(Normalmap, normalmapCoord0).xzy;
	vec3 normalMapLayer1 = texture(Normalmap, normalmapCoord1).xzy;
	vec3 normalMapNormal = mix(normalMapLayer0, normalMapLayer1, distortionBlend);
		// scale with speed
	float flowSpeedSq = dot(flow, flow);
	float flowSpeed = sqrt(flowSpeedSq);
	normalMapNormal = normalMapNormal * 2.0 - vec3(1.0);
	normalMapNormal.y /= SpeedToNormalDistortion * flowSpeed;

	// Final Normal
	vec3 normal = normalize(ComputeHeightmapNormal(inHeightmapCoord) + normalMapNormal * 0.25);

	// vector to camera and camera distance
	vec3 toCamera = CameraPosition - inWorldPos;
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
	float specularAmount = pow(saturate(dot(cameraDirReflection, GlobalDirLightDirection)), 8.0);
	specularAmount *= fresnel;



	// Water depth
	vec4 terrainInfo = texture(TerrainInfo, inHeightmapCoord);
	float waterDepth = terrainInfo.a;

	// Needed: amout of water in screenspace on this very pixel
	// This would need raymarching... instead just use this self-made approximation
	//float waterViewSpaceDepth = waterDepth / saturate(nDotV + 0.1);


	// Refraction
	vec3 projectiveTexcoords = inProjectiveCoord.xyz / inProjectiveCoord.w;
	vec3 refractionTexture;
	vec3 refractionColor = ComputeRefractionColor(nDotV, nDotL, toCamera, normal, waterDepth, projectiveTexcoords.xy, refractionTexture);
	

	// Screenspace local reflection.
	//bool terrainReflection = false;
	/*vec3 raymarchPos = inWorldPos;
	for(float t=1.05; t<50.0; t*=1.1f)	// ~40 iterations
	{
		raymarchPos += cameraDirReflection * t;
		vec4 terrainInfo = textureLod(TerrainInfoFiltered, raymarchPos.xz * HeightmapWorldTexelSize, 3);
		if(terrainInfo.x > raymarchPos.y)
		{
			reflectionColor /= CalcLuminance(reflectionColor) * 40.0;	// fake Reflections by darkening the reflection color
	//		terrainReflection = true;
			break;
		}
	}*/



//	

	// Transform reflection vector to viewspace
	vec4 reflectedPos = (ViewProjection * vec4(inWorldPos + cameraDirReflection, 1.0));
	reflectedPos.xyz /= reflectedPos.w;
	reflectedPos.xy = reflectedPos.xy * 0.5 + vec2(0.5);
	vec3 reflectionScreenSpace = reflectedPos.xyz - projectiveTexcoords.xyz;
	
	float screenSpaceReflectionIntensity = 0.0f;
	vec2 screenSpaceReflectionTexcoord = vec2(0,0);

	if(reflectionScreenSpace.z > 0)
	{
		if(reflectionScreenSpace.y > 0)
		{
			if(reflectionScreenSpace.x > 0)
				screenSpaceReflectionIntensity = float(MaxMapBasedDepthBufferRaymarching(projectiveTexcoords, reflectionScreenSpace, screenSpaceReflectionTexcoord, 0));
			else
				screenSpaceReflectionIntensity = float(MaxMapBasedDepthBufferRaymarching(projectiveTexcoords, reflectionScreenSpace, screenSpaceReflectionTexcoord, XNEG));
		}
		else
		{
			if(reflectionScreenSpace.x > 0)
				screenSpaceReflectionIntensity = float(MaxMapBasedDepthBufferRaymarching(projectiveTexcoords, reflectionScreenSpace, screenSpaceReflectionTexcoord, YNEG));
			else
				screenSpaceReflectionIntensity = float(MaxMapBasedDepthBufferRaymarching(projectiveTexcoords, reflectionScreenSpace, screenSpaceReflectionTexcoord, XNEG | YNEG));
		}
	}

	vec3 reflectionColor;
	if(screenSpaceReflectionIntensity != 0.0f)
	{
		vec2 ssReflectionCoordToBorder = vec2(1.0) - screenSpaceReflectionTexcoord * 2.0f;
		ssReflectionCoordToBorder *= ssReflectionCoordToBorder;
		float borderFade = dot(ssReflectionCoordToBorder, ssReflectionCoordToBorder);
		screenSpaceReflectionIntensity *= 1.0 - borderFade * borderFade;
		//screenSpaceReflectionIntensity *= 1-reflectionScreenSpace.z;


		vec3 screenSpaceReflectionColor = textureLod(SceneTexture, screenSpaceReflectionTexcoord.xy, 0.0).rgb;	
		vec3 cubemapReflectionColor = textureLod(ReflectionCubemap, cameraDirReflection, 0.0).rgb;	

		reflectionColor = mix(cubemapReflectionColor, screenSpaceReflectionColor, screenSpaceReflectionIntensity);
	}
	else
	{
		reflectionColor = textureLod(ReflectionCubemap, cameraDirReflection, 0.0).rgb;
	}

	

//reflectionColor = pow(texture(SceneDepthTexture, projectiveTexcoords.xy).rrr, vec3(10000));

	// Combine Refraction & Reflection & Specular
	vec3 color = mix(refractionColor, reflectionColor, saturate(fresnel)) + GlobalDirLightColor * specularAmount;
	
	// Shore hack against artifacts
	float pixelToGround = inWorldPos.y - terrainInfo.r;	// this is a bit better than the classic terrainInfo.a approach
	color = mix(refractionTexture, color, saturate(pixelToGround*pixelToGround * 0.25));

	// Foam for fast water - just reuse the same coord and technique from the normalmaps
	const float SpeedToFoamBlend = 0.00004;
	vec4 foam0 = texture(Foam, normalmapCoord0);
	vec4 foam1 = texture(Foam, normalmapCoord1);
	vec4 foam = mix(foam0, foam1, distortionBlend);
	color = mix(color, foam.rgb, saturate(flowSpeedSq*SpeedToFoamBlend) * foam.a);

	// Fogging
	color = ApplyFog(color, CameraPosition, cameraDistance, toCamera);

	// Color output
	FragColor.rgb = color;
	FragColor.a = 1.0;
}