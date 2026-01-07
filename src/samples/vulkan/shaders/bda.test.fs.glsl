#version 460 core

#include "core/bindless.core.glsl"

in VS_OUT_FS_IN {
	layout (location = 0) vec2 uv;
} fs_in;

layout (location = 0) out vec4 FinalFragColor;

void main() {
	FinalFragColor = vec4(fs_in.uv,
						  // clamp(fs_in.uv.x + fs_in.uv.y, 0.0f, 1.0f),
						  0.0f,
						  1.0f);
}
