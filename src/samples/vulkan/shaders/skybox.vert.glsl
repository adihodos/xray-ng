#version 460 core

#include "core/bindless.core.glsl"

#if defined(__SKYBOX_REAL_CUBE__)
layout (location = 0) in vec3 vpos;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec3 tg;
layout (location = 3) in vec2 texc;
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
	const mat4 skybox_tf = mat4(
        vec4(5000.0, 0.0, 0.0, 0.0),
        vec4(0.0, 5000.0, 0.0, 0.0),
        vec4(0.0, 0.0, 5000.0, 0.0),
        vec4(fgd.eye_pos, 1.0));

	gl_Position = (fgd.world_view_proj * skybox_tf * vec4(vpos, 1.0)).xyww;
#else
	vs_out.texcoords = mat3(fgd.view) * vec3(QUAD_VERTICES[gl_VertexIndex], 1.0);
	gl_Position = vec4(QUAD_VERTICES[gl_VertexIndex], 1.0, 1.0);
#endif
}
