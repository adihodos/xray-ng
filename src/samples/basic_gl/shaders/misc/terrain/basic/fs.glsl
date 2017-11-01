#version 450 core

in VS_OUT_FS_IN {
  vec3 pos_view;
  vec3 norm_view;
  vec2 texcoords;
} fs_in;

layout (location = 0) out vec4 final_frag_color;

void main() {
  final_frag_color = vec4(0.1f, 0.8f, 0.1f, 1.0f);
}
