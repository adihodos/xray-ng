#version 460 core

#extension GL_EXT_nonuniform_qualifier : require

layout (local_size_x = 8, local_size_y = 8) in;

layout (set = 0, binding = 0, rgba8ui) writeonly uniform uimage2D img_output[];

layout (push_constant) uniform PushConstantsGlobal {
	uint packed0;
	uint packed1;
} g_pushConsts;

void main() {
	imageStore(img_output[(g_pushConsts.packed0 >> 8) & 0xFF],
			   ivec2(gl_GlobalInvocationID.xy),
			   uvec4(gl_GlobalInvocationID.x % 64, gl_GlobalInvocationID.y % 64, 0.0, 0.0));
}
