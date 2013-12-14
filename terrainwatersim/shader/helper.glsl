vec3 ApplyFog(vec3 color, float cameraDistance, vec3 toCameraVec)
{
	const float fogDensity = 0.001;
	const float fogIntensity = 0.6;
	float fogAmount = clamp( - fogIntensity * exp(-CameraPosition.y * fogDensity) * (1.0 - exp( cameraDistance * toCameraVec.y * fogDensity)) / toCameraVec.y, 0, 1);
	vec3 fogColor = vec3(0.18867780436772762, 0.4978442963618773, 0.6616065586417131); // air color
	return mix(color, fogColor, fogAmount);
}

float Fresnel(float nDotV, float R0)
{
	float base = 1.0 - nDotV;
	float exponential = pow(base, 5.0);
	return exponential + R0 * (1.0 - exponential);
}

vec3 Refract(float enterDotNormal, vec3 enteringRay, vec3 normal, float eta)
{
    float k = 1.0 - eta * eta * (1.0 - enterDotNormal * enterDotNormal);
    if (k < 0.0)
        return vec3(0.0);
    else
        return eta * enteringRay - (eta * enterDotNormal + sqrt(k)) * normal;
}

vec3 Refract(vec3 enteringRay, vec3 normal, float eta)
{
	float cosi = dot(enteringRay, normal);
	return Refract(cosi, enteringRay, normal, eta);
}

vec3 ComputeRayDirection(in vec2 screenTexCor, in mat4 inverseViewProjection)
{
	vec2 deviceCor = 2.0 * screenTexCor - 1.0;
	vec4 rayOrigin = inverseViewProjection * vec4(deviceCor, -1.0, 1.0);
	rayOrigin.xyz /= rayOrigin.w;
	vec4 rayTarget = inverseViewProjection * vec4(deviceCor, 0.0, 1.0) ;
	rayTarget.xyz /= rayTarget.w;
	return normalize(rayTarget.xyz - rayOrigin.xyz);
};

#define saturate(value) clamp((value), 0, 1)


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