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
	vec3 vecdz = vec3(0.0f, h[1] - h[0], worldStep);	// worldStep*2 would be correct? But this looks muuuuuch nicer!
	vec3 vecdx = vec3(worldStep, h[2] - h[3], 0.0f);
	return normalize(cross(vecdx, vecdz));
}

void main()
{
	vec3 normal = ComputeNormal(In.HeightmapCoord);

	float lighting = clamp(dot(normal, GlobalDirLightDirection), 0, 1);
	
	// good old backlighting hack!
	vec3 backLightDir = GlobalDirLightDirection;
	backLightDir.xz = -backLightDir.xz;
	float ambientLightAmount = clamp(dot(normal, backLightDir), 0, 1);


	// texturing
	/*vec3 texcoord3D = In.WorldPos*0.1f;
	vec3 textureWeights = abs(normal);
	textureWeights.y *= textureWeights.y;
	textureWeights /= textureWeights.x + textureWeights.y + textureWeights.z;
	
	vec3 textureY = texture(TextureY, texcoord3D.xz).xyz;
	vec3 textureZ = texture(TextureXZ, texcoord3D.xy).xyz;
	vec3 textureX = texture(TextureXZ, texcoord3D.zy).xyz;

	vec3 diffuseColor = textureX * textureWeights.x + textureY * textureWeights.y + textureZ * textureWeights.z;
	*/
	vec3 diffuseColor = vec3(1.0f);


	FragColor.xyz = diffuseColor * (GlobalDirLightColor * lighting + GlobalAmbient * ambientLightAmount);

	// Normal debugging:
	//FragColor.xyz = abs(vec3(normal.x, normal.y,normal.z));
	FragColor.a = 1.0f;
}
