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

float simplex(vec3 pos, float seed) {
	return (snoise(vec3(pos + seed)) + 1.0) * 0.5;
}

float baseNoise(vec3 pos, float frq, float seed ) {
	const int octaves = 16;
	float amp = 0.5;

	float n = 0.0;
	float gain = 1.0;
	
	for(int i = 0; i < octaves; i++) {
		n += simplex(vec3(pos.x*gain/frq, pos.y*gain/frq, pos.z*gain/frq), seed+float(i)*10.0) * amp/gain;
		gain *= 2.0;
	}

	// increase contrast
	n = ( (n - 0.5) * 3.0 ) + 0.6;
	return n;
}

const uint IMG_SIZE_MASK = 0xFFFF;

void main() {
	const uint img_output_id = g_PushConsts.packed0 & 0xFF;
	const uint resolution = (g_PushConsts.packed0 >> 8) & IMG_SIZE_MASK;
	const uint face_index = (g_PushConsts.packed0 >> 24);
	const uint seed = g_PushConsts.packed1 & 0xFFFF;

	const vec2 uv = vec2(float(gl_GlobalInvocationID.x) / float(resolution), 1.0 - float(gl_GlobalInvocationID.y) / float(resolution));
	const vec3 sphericalCoord = getSphericalCoord(face_index, uv.x * float(resolution), uv.y * float(resolution), float(resolution));

	//
	// create base stars
	const vec2 F = worley((sphericalCoord * 200.0) + vec3(seed), 1.0, true);
	float n = F.x;
	n = 1.0 - n;
	n *= 1.2;
	n = pow(n, 4.0);

	float sub1 = baseNoise(sphericalCoord, 0.003, seed+32.284);
	n *= sub1;

	imageStore(img_output[img_output_id], ivec2(gl_GlobalInvocationID.xy), vec4(vec3(n), 1.0));
}
