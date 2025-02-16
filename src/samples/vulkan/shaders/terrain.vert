#version 460 core

#include "core/bindless.core.glsl"
#include "core/std.interface.glsl"

#if defined(__VERT_SHADER__)

layout (location = 0) in vec3 vs_pos;
layout (location = 1) in vec2 vs_uv;

out gl_PerVertex {
	vec4 gl_Position;
};

#endif

INTERFACE_BLOCK
VS_OUT_FS_IN {
	layout (location = 0) flat uint colormap;
	layout (location = 1) vec2 uv;
} BLOCKVAR;

#if defined(__VERT_SHADER__)
void main() {
	const uint frame_idx = (g_GlobalPushConst.data) & 0xFF;
	const uint inst_buffer_idx = (g_GlobalPushConst.data & 0xFFFF0000) >> 16;
	const uint instance_index = (g_GlobalPushConst.data & 0x0000FF00) >> 8;
	const FrameGlobalData_t fgd = g_FrameGlobal[frame_idx].data[0];
	const TerrainInstanceData terrain = g_TerrainInstancesGlobal[inst_buffer_idx].data[instance_index];
	const float height = texture(g_Textures2DGlobal[terrain.heightmap], vs_uv).r;

	gl_Position = fgd.world_view_proj
		//terrain.wvp 
		* vec4(vs_pos.x, height, vs_pos.z, 1.0);
	vs_out.colormap = terrain.colormap;
	vs_out.uv = vs_uv;
}
#else
layout (location = 0) out vec4 FinalFragColor;

void main() {
	FinalFragColor = texture(g_Textures2DGlobal[fs_in.colormap], fs_in.uv);
}
#endif
