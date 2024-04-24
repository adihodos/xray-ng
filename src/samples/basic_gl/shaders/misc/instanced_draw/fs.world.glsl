#version 450 core

in VS_OUT_FS_IN {
	// vec3 pos;
	// vec3 normal;
	// vec2 texcoord;
	vec3 color;
} fs_in;

out vec4 final_frag_color;

void main() {
	// if (fs_in.pos.y <= 0.0f) {
	//   final_frag_color = vec4(0.0f, 0.0f, 1.0f, 0.0f);
	// } else if (fs_in.pos.y <= 3.0f) {
	//   final_frag_color = vec4(0.0f, 1.0f, 0.0f, 1.0f);
	// } else if (fs_in.pos.y <= 6.0f) {
	//   final_frag_color = vec4(0.0f, 1.0f, 1.0f, 1.0f);
	// } else {
	//   final_frag_color = vec4(1.0f, 1.0f, 1.0f, 1.0f);
	// }

	final_frag_color = vec4(fs_in.color, 1.0);
}
