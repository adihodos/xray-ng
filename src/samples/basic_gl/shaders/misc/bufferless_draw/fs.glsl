#version 450 core

in GS_OUT_FS_IN {
  vec4 color;
} fs_in;

out vec4 FinalFragColor;

void main() {
  FinalFragColor = fs_in.color;
}