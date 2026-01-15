#version 460 core

#include "core/bindless.core.glsl"
#include "core/std.interface.glsl"

#if defined(__VERT_SHADER__)

layout (location = 0) in vec2 vs_in_pos;
layout (location = 1) in vec2 vs_in_uv;
layout (location = 2) in uint vs_in_texid;
layout (location = 3) in uint vs_in_color;

out gl_PerVertex {
	vec4 gl_Position;
};

#endif

INTERFACE_BLOCK
VS_OUT_FS_IN {
	layout(location = 0) vec4 color;
	layout(location = 1) vec2 uv;
	layout(location = 2) flat uint texture_idx;
	layout(location = 3) flat uint array_elem_idx;
} BLOCKVAR;

#if defined(__VERT_SHADER__)
void main() {
	const uint frame_idx = (g_GlobalPushConst.data) & 0xFF;
	const FrameGlobalData_t fgd = g_FrameGlobal[frame_idx].data[0];
	const uint texture_index = (g_GlobalPushConst.data & 0xFFFF0000) >> 16;

	gl_Position = vec4((fgd.ortho_proj * vec4(vs_in_pos, 0.0, 1.0)).xy, 0.0, 1.0);
	vs_out.color = unpackUnorm4x8(vs_in_color);
	vs_out.uv = vs_in_uv;
	vs_out.texture_idx = texture_index;
	vs_out.array_elem_idx = vs_in_texid;
}
#else
layout (location = 0) out vec4 FinalFragColor;

void main() {
	FinalFragColor = vec4(fs_in.color.rgb, texture(g_Textures2DArrayGlobal[fs_in.texture_idx], vec3(fs_in.uv, fs_in.array_elem_idx)).r);
}
#endif
