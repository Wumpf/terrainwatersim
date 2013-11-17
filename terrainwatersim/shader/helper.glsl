vec3 ComputeRayDirection(in vec2 screenTexCor, in mat4 inverseViewProjection)
{
	vec2 deviceCor = 2.0f * screenTexCor - 1.0f;
	vec4 rayOrigin = inverseViewProjection * vec4(deviceCor, -1.0f, 1.0f);
	rayOrigin.xyz /= rayOrigin.w;
	vec4 rayTarget = inverseViewProjection * vec4(deviceCor, 0.0f, 1.0f) ;
	rayTarget.xyz /= rayTarget.w;
	return normalize(rayTarget.xyz - rayOrigin.xyz);
}