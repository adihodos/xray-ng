#version 460 core

#include "bindless.core.glsl"

out gl_PerVertex {
	vec4 gl_Position;
};

out VS_OUT_FS_IN {
	layout (location = 0) vec2 uv;
} vs_out;

void main() {
	//
	// https://www.saschawillems.de/blog/2016/08/13/vulkan-tutorial-on-rendering-a-fullscreen-quad-without-buffers
	vs_out.uv = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
    gl_Position = vec4(vs_out.uv * 2.0f + -1.0f, 0.0f, 1.0f);
}
