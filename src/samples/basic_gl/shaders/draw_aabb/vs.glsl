#version 450 core

out VS_OUT_GS_IN {
  vec3 pos;
} vs_out;

void main() {
  vs_out.pos = vec3(float(gl_VertexID));
}