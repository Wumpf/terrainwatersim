vec3 ComputeRayDirection(in vec2 screenTexCor, in mat4 inverseViewProjection)
{
	vec2 deviceCor = 2.0f * screenTexCor - 1.0f;
	vec4 rayOrigin = inverseViewProjection * vec4(deviceCor, -1.0f, 1.0f);
	rayOrigin.xyz /= rayOrigin.w;
	vec4 rayTarget = inverseViewProjection * vec4(deviceCor, 0.0f, 1.0f) ;
	rayTarget.xyz /= rayTarget.w;
	return normalize(rayTarget.xyz - rayOrigin.xyz);
}

/*
vec4 interpolateQuad(in vec4 v0, in vec4 v1, in vec4 v2, in vec4 v3)
{
	vec4 a = mix(v0, v1, gl_TessCoord.x);
	vec4 b = mix(v3, v2, gl_TessCoord.x);
	return mix(a, b, gl_TessCoord.y);
}

vec3 interpolateQuad(in vec3 v0, in vec3 v1, in vec3 v2, in vec3 v3)
{
	vec3 a = mix(v0, v1, gl_TessCoord.x);
	vec3 b = mix(v3, v2, gl_TessCoord.x);
	return mix(a, b, gl_TessCoord.y);
}

vec2 interpolateQuad(in vec2 v0, in vec2 v1, in vec2 v2, in vec2 v3)
{
	vec2 a = mix(v0, v1, gl_TessCoord.x);
	vec2 b = mix(v3, v2, gl_TessCoord.x);
	return mix(a, b, gl_TessCoord.y);
}
*/