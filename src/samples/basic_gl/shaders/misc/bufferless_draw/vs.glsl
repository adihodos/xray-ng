#version 450 core

out VS_OUT_GS_IN {
  int vertexid;
} vs_out;

void main() {
  vs_out.vertexid = gl_VertexID;
}