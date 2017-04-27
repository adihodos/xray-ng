#version 450 core

layout (binding = 0) uniform sampler2D IMAGE_TEX;

in VS_OUT_PS_IN {
  vec3 view_pos;
  vec3 view_norm;
  vec2 texcoord;
} ps_in;

layout (location = 0) out vec4 FinalFragColor;

void main() {
  FinalFragColor = texture(IMAGE_TEX, ps_in.texcoord);
}