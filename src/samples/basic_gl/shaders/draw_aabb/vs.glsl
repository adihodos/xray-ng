#version 450 core

layout (location = 0) in vec3 vs_in_pos;

out VS_OUT_GS_IN {
  vec3 pos;
} vs_out;

void main() {
  vs_out.pos = vs_in_pos;
}