#version 450 core

out gl_PerVertex {
  vec4 gl_Position;
};

layout (location = 0) in vec2 vs_in_pos;
layout (location = 1) in vec2 vs_in_texcoord;
uniform int WRAPPING_FACTOR;

out VS_OUT_PS_IN {
  layout (location = 0) vec2 texcoord;
} vs_out;

void main() {
  gl_Position = vec4(vs_in_pos, 0.0f, 1.0f);
  vs_out.texcoord = vs_in_texcoord * float(WRAPPING_FACTOR);
}