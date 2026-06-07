#ifndef _NOISE_BASE_GLSL_
#define _NOISE_BASE_GLSL_
//
// Description : Array and textureless GLSL 2D/3D/4D simplex
//               noise functions.
//      Author : Ian McEwan, Ashima Arts.
//  Maintainer : ijm
//     Lastmod : 20110822 (ijm)
//     License : Copyright (C) 2011 Ashima Arts. All rights reserved.
//               Distributed under the MIT License. See LICENSE file.
//               https://github.com/ashima/webgl-noise
//

float mod289(float x) {
	return x - floor(x * (1.0 / 289.0)) * 289.0;
}

vec2 mod289(vec2 x) {
	return x - floor(x * (1.0 / 289.0)) * 289.0;
}

vec3 mod289(vec3 x) {
	return x - floor(x * (1.0 / 289.0)) * 289.0;
}

vec4 mod289(vec4 x) {
	return x - floor(x * (1.0 / 289.0)) * 289.0;
}

float permute(float x) {
     return mod289(((x*34.0)+1.0)*x);
}

vec3 permute(vec3 x) {
    return mod((34.0 * x + 1.0) * x, 289.0);
}

vec3 dist(vec3 x, vec3 y,  bool manhattanDistance) {
	return manhattanDistance ?  abs(x) + abs(y) :  (x * x + y * y);
}

vec3 dist(vec3 x, vec3 y, vec3 z,  bool manhattanDistance) {
  return manhattanDistance ?  abs(x) + abs(y) + abs(z) :  (x * x + y * y + z * z);
}

vec4 permute(vec4 x) {
	return mod289(((x*34.0)+1.0)*x);
}

float taylorInvSqrt(float r)
{
	return 1.79284291400159 - 0.85373472095314 * r;
}

vec4 taylorInvSqrt(vec4 r)
{
	return 1.79284291400159 - 0.85373472095314 * r;
}

vec4 grad4(float j, vec4 ip)
{
	const vec4 ones = vec4(1.0, 1.0, 1.0, -1.0);
	vec4 p,s;

	p.xyz = floor( fract (vec3(j) * ip.xyz) * 7.0) * ip.z - 1.0;
	p.w = 1.5 - dot(abs(p.xyz), ones.xyz);
	s = vec4(lessThan(p, vec4(0.0)));
	p.xyz = p.xyz + (s.xyz*2.0 - 1.0) * s.www;

	return p;
}

vec3 getSphericalCoord(uint index, float x, float y, float width) {
	width /= 2.0;
	x -= width;
	y -= width;

	const vec3 coords[6] = {
		{width, -y, -x},
		{-width, -y, x},
		{x, width, y},
		{x, -width, -y},
		{x, -y, width},
		{-x, -y, -width}
	};
	return normalize(coords[index]);
}

#endif // !defined _NOISE_BASE_GLSL_
