#version 460 core

#include "core/bindless.core.glsl"


in VS_OUT_FS_IN {
	layout (location = 0) vec2 uv;
} fs_in;

layout (location = 0) out vec4 FinalFragColor;

void main() {
	const uint tex_id = (g_GlobalPushConst.data & 0xFFFF0000) >> 16;
	FinalFragColor = texture(g_Textures2DGlobal[tex_id], fs_in.uv);
		// vec4(fs_in.uv,
						  // clamp(fs_in.uv.x + fs_in.uv.y, 0.0f, 1.0f),
						  // 0.0f,
						  // 1.0f);
}
