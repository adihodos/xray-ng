#version 460 core

#include "core/bindless.core.glsl"

#if defined(__SKYBOX_REAL_CUBE__)
layout (location = 0) in vec3 vpos;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec3 tg;
layout (location = 3) in vec3 texc;
#else
const vec2 QUAD_VERTICES[] = {
	{1.0, -1.0},
	{1.0, 1.0},
	{-1.0, 1.0},
	{-1.0, 1.0},
	{-1.0, -1.0},
	{1.0, -1.0}
};

#endif

layout (location = 0) out VS_OUT_FS_IN {
	vec3 texcoords;
	flat uint cubemap;
} vs_out;

void main() {
	const FrameGlobalData_t fgd = g_FrameGlobal[g_GlobalPushConst.data & 0xFF].data[0];
	vs_out.cubemap = g_GlobalPushConst.data >> 8;
	
#if defined(__SKYBOX_REAL_CUBE__)
	vs_out.texcoords = vpos;
		// mat3(fgd.view) * vpos;
	gl_Position = (fgd.world_view_proj * vec4(vpos, 1.0)).xyww;
		// (fgd.projection * fgd.view) * vec4(vpos, 1.0);
#else
	vs_out.texcoords = mat3(fgd.view) * vec3(QUAD_VERTICES[gl_VertexIndex], 1.0);
	gl_Position = vec4(QUAD_VERTICES[gl_VertexIndex], 1.0, 1.0);
#endif

	// const vec2 uv = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
	// vs_out.texcoords = mat3(fgd.view) * vec3(uv, 1.0);
    // gl_Position = vec4(uv * 2.0f + -1.0f, 1.0f, 1.0f);
}
