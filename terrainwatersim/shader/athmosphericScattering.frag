#version 430

#include "constantbuffers.glsl"
#include "helper.glsl"

// input
layout(location = 0) in vec3 gs_out_direction;

// output
layout(location = 0, index = 0) out vec4 fragColor;



// Kudos to this great pice of work http://codeflow.org/entries/2011/apr/13/advanced-webgl-part-2-sky-rendering/
// Did only minor modifications:
// * No horizon extinction
// * Other default values
/*
	:copyright: 2011 by Florian Boesch <pyalot@gmail.com>.
	:license: GNU AGPL3
*/

// a bit more artistic

const vec3 Kr = vec3(0.15, 0.3f, 0.6f);
// vec3(0.18867780436772762, 0.4978442963618773, 0.6616065586417131); // nitrogenium absorption color


const float spot_brightness = 1000;
const float scatter_strength = 28.0f/1000;

const float rayleigh_brightness = 3.1f;
const float rayleigh_strength = 0.3f; // 0.139
const float rayleigh_collection_power = 0.4f;	// 0.51

const float mie_brightness = 0.2f;
const float mie_strength = 0.0264f;
const float mie_collection_power = 0.39f;
const float mie_distribution = 0.63f;
	
 
const float surface_height = 0.994;
const float intensity = 1.8;
#define step_count 16

 
float atmospheric_depth(vec3 position, vec3 dir)
{
	 float a = dot(dir, dir);
	 float b = 2.0*dot(dir, position);
	 float c = dot(position, position)-1.0;
	 float det = b*b-4.0*a*c;
	 float detSqrt = sqrt(det);
	 float q = (-b - detSqrt)/2.0;
	 float t1 = c/q;
	 return t1;
};

float phase(float alpha, float g)
{
	 float a = 3.0*(1.0-g*g);
	 float b = 2.0*(2.0+g*g);
	 float c = 1.0+alpha*alpha;
	 float d = pow(1.0+g*g-2.0*g*alpha, 1.5);
	 return (a/b)*(c/d);
};

vec3 absorb(float dist, vec3 color, float factor)
{
	return color-color*pow(Kr, vec3(factor/dist));
}

void main()
{
	vec3 eyedir = normalize(gs_out_direction);
	
	float alpha = dot(eyedir, GlobalDirLightDirection);
	 
	float rayleigh_factor = phase(alpha, -0.01)*rayleigh_brightness;
	float mie_factor = phase(alpha, mie_distribution)*mie_brightness;
	float spot = smoothstep(0.0, 15.0, phase(alpha, 0.9995))*spot_brightness;

	vec3 eye_position = vec3(0.0, surface_height, 0.0);
	float eye_depth = atmospheric_depth(eye_position, eyedir);
	float step_length = eye_depth / float(step_count);

	vec3 rayleigh_collected = vec3(0.0, 0.0, 0.0);
	vec3 mie_collected = vec3(0.0, 0.0, 0.0);

	for(int i=0; i<step_count; ++i)
	{
		float sample_distance = step_length*float(i);
		vec3 position = eye_position + eyedir*sample_distance;

		float sample_depth = atmospheric_depth(position, GlobalDirLightDirection);
		vec3 influx = absorb(sample_depth, vec3(intensity), scatter_strength);
		rayleigh_collected += absorb(sample_distance, Kr*influx, rayleigh_strength);
		mie_collected += absorb(sample_distance, influx, mie_strength);
	}

	rayleigh_collected = (rayleigh_collected * pow(eye_depth, rayleigh_collection_power))/float(step_count);
	mie_collected = (mie_collected * pow(eye_depth, mie_collection_power))/float(step_count);

	vec3 color = vec3(spot*mie_collected + mie_factor*mie_collected + rayleigh_factor*rayleigh_collected);

	fragColor = vec4(color, 1.0f);
}