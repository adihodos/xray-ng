#version 450 core

in VS_OUT_FS_IN {
  vec3 pos;
  vec3 normal;
  vec4 color;
} fs_in;

out vec4 final_frag_color;

void main() {
  final_frag_color = fs_in.color;
}
