#version 450 core

layout (location = 0) in vec3 vs_in_pos;
layout (location = 1) in vec3 vs_in_normal;
layout (location = 2) in vec2 vs_in_texcoord;

out VS_OUT_GS_IN {
  vec3 pos;
  vec3 normal;
} vs_out;

void main() {
  vs_out.pos = vs_in_pos;
  vs_out.normal = vs_in_normal;
}
