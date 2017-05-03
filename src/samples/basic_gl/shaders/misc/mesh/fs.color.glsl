#version 450 core

in GS_OUT_PS_IN {
  vec4 color;
} ps_in;

layout (location = 0) out vec4 FinalFragColor;

void main() {
  FinalFragColor = ps_in.color;
}