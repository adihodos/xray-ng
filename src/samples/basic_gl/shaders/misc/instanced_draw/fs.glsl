#version 450 core

in VS_OUT_FS_IN {
  vec3 pos;
  vec3 normal;
  vec2 texcoord;
  uint texid;
} fs_in;

layout (location = 0) out vec4 final_frag_color;

void main() {
  final_frag_color = vec4(vec3(float(fs_in.texid) / 8.0f), 1.0f);
}
