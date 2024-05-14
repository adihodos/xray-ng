#version 450 core

in VS_OUT_FS_IN {
	vec3 pos;
	vec3 normal;
	vec2 texcoord;
	flat uint inst_id;
} fs_in;

layout (location = 0) out vec4 final_frag_color;
layout (binding = 0) uniform sampler2DArray TEXTURES;

void main() {
	final_frag_color = texture(TEXTURES, vec3(fs_in.texcoord, float(fs_in.inst_id % 10)));
}
