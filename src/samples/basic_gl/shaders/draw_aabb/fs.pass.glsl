#version 450 core

in GS_OUT_FS_IN {
  vec4 color;
} fs_in;

layout (location = 0) out vec4 FinalFragColor;

void main() {
  FinalFragColor = vec4(1.0f, 0.0f, 0.0f, 1.0f); 
  //fs_in.color;
}