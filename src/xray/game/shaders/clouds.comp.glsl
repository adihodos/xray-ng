#version 460 core

#extension GL_EXT_nonuniform_qualifier : require

#include "core/noise/noise.simplex.glsl"
#include "core/noise/noise.worley.glsl"

layout (local_size_x = 8, local_size_y = 8) in;
layout (set = 0, binding = 0, rgba8) writeonly uniform image2D img_output[];

layout (push_constant) uniform PushConstantsGlobal {
	uint packed0;
	uint packed1;
} g_PushConsts;

float simplexRidged(vec3 pos, float seed) {
	return abs(snoise(vec4(pos, seed)));
}

float simplex(vec3 pos, float seed) {
	return (snoise(vec4(pos, seed)) + 1.0) * 0.5;
}

float baseNoise(vec3 pos, float frq, float seed ) {
	const int octaves = 16;
	const float amp = 0.5;

	float n = 0.0;
	float gain = 1.0;
	for (int i = 0; i < octaves; i++) {
		n +=  simplexRidged(vec3(pos.x*gain/frq, pos.y*gain/frq, pos.z*gain/frq), seed+float(i)*10.0) * amp/gain;
		gain *= 2.0;
	}

	n = ( (n - 0.5) * 2.0 ) + 0.6;
	return n;
}

float ridgedNoise(vec3 pos, float frq, float seed) {
	const int octaves = 16;
	const float amp = 0.5;

	float n = 0.0;
	float gain = 1.0;
	for (int i = 0; i < octaves; i++) {
		n +=  simplexRidged(vec3(pos.x*gain/frq, 2.0*pos.y*gain/frq, pos.z*gain/frq), seed+float(i)*10.0) * amp/gain;
		gain *= 2.0;
	}

	n = 1.0-n;
	n = pow(n, 6.0);
	n = 1.0-n;

	return n;
}

float billowNoise(vec3 pos, float frq, float seed) {
	const int octaves = 16;
	const float amp = 0.5;

	float n = 0.0;
	float gain = 1.0;
	for (int i = 0; i < octaves; i++) {
		n +=  simplexRidged(vec3(pos.x*gain/frq, pos.y*gain/frq, pos.z*gain/frq), seed+float(i)*10.0) * amp/gain;
		gain *= 2.0;
	}

	n = 1.0-n;
	n = pow(n, 1.0);
	n = 1.0-n;

	return n;
}

float cloud(vec3 pos, float seed) {
	float n = snoise(vec4(pos, seed));
	n = sin(n*3.0);

	n = n*0.5 + 0.5;
	return n;
}

float cloudNoise(vec3 pos, float frq, float seed) {
	const int octaves = 32;
	float amp = 0.5;

	float n = 0.0;
	float gain = 1.0;
	for (int i = 0; i < octaves; i++) {
		n +=  cloud(vec3(pos.x*gain/frq, 1.0*pos.y*gain/frq, pos.z*gain/frq), seed+float(i)*10.0) * amp/gain;
		gain *= 2.0;
	}

	n = 1.0-n;
	n = pow(n, 1.0);
	n = 1.0-n;

	return n;
}

float perlin(vec3 pos, float seed) {
	float n = snoise(vec4(pos, seed));
	n = (n + 1.0) * 0.5;
	return n;
}

float perlinNoise(vec3 pos, float frq, float seed ) {

	const int octaves = 16;
	float n = 0.0;
	float amplitude = 0.7;

	for (int i = 0; i < octaves; i++) {
		n += amplitude * perlin(pos*frq, seed);
		frq *= 2.0; // lacunarity = 2.0
		amplitude *= n; // gain = 0.5
	}

	n = pow(n, 2.0);

	return n;
}

void main() {

#include "comp.unpack.pushconsts.glsl"
	
	float sub1 = baseNoise(sphericalCoord, 0.5, seed);
	float sub1b = billowNoise(sphericalCoord, 1.0, seed+93.386);
	float n1 = baseNoise(sphericalCoord + vec3(sub1*0.3), 0.5, seed+38.378);

	float sub2 = baseNoise(sphericalCoord, 0.5, seed+12.412);
	float n2 = baseNoise(sphericalCoord + vec3(sub2*0.3), 0.5+sub1b, seed+58.578);

	n1 = 1.0-n1;
	n1 *= 0.1;
	n1 = 1.0-n1;

	n2 = 1.0-n2;
	n2 = pow(n2, 5.0);
	n2 *= 0.3;
	n2 = 1.0-n2;
	n2 = pow(n2, 5.0);

	imageStore(img_output[img_output_id], ivec2(gl_GlobalInvocationID.xy), vec4(vec3(n1), n2));
}
