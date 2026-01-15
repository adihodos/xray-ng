#version 460 core

#include "core/bindless.core.glsl"

layout (location = 0) out VS_OUT_FS_IN {
	vec3 texcoords;
	flat uint cubemap;
} vs_out;

const vec2 QUAD_VERTICES[] = {
	{1.0, -1.0},
	{1.0, 1.0},
	{-1.0, 1.0},
	{-1.0, 1.0},
	{-1.0, -1.0},
	{1.0, -1.0}
};

void main() {
	const FrameGlobalData_t fgd = g_FrameGlobal[g_GlobalPushConst.data & 0xFF].data[0];
	vs_out.texcoords = mat3(fgd.view) * vec3(QUAD_VERTICES[gl_VertexIndex], 1.0);
	vs_out.cubemap = g_GlobalPushConst.data >> 8;
	gl_Position = vec4(QUAD_VERTICES[gl_VertexIndex], 1.0, 1.0);

	// const vec2 uv = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
	// vs_out.texcoords = mat3(fgd.view) * vec3(uv, 1.0);
    // gl_Position = vec4(uv * 2.0f + -1.0f, 1.0f, 1.0f);
}
