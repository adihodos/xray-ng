#version 460 core

#extension GL_EXT_nonuniform_qualifier : require

layout (local_size_x = 8, local_size_y = 8) in;

layout (set = 0, binding = 0, rgba8) writeonly uniform image2D img_output[];

layout (push_constant) uniform PushConstantsGlobal {
	uint packed0;
	uint packed1;
} g_PushConsts;

void main() {
	const uint x = gl_GlobalInvocationID.x + g_PushConsts.packed1;
	const uint y = gl_GlobalInvocationID.y + g_PushConsts.packed1;
	const uint color = g_PushConsts.packed1;
	
	imageStore(img_output[g_PushConsts.packed0],
			   ivec2(gl_GlobalInvocationID.xy),			   
			   vec4((float(x % 255) / 255.0),
					(float(y % 255) / 255.0),
					(float((x + y) % 255) / 255.0), 1.0)
			   );
}
