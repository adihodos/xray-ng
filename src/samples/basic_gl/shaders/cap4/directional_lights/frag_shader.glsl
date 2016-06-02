#version 450 core
#pragma debug(on)
#pragma optimize(off)

in PS_IN {
  layout (location = 0) vec4 frag_color;
} ps_in;

layout (location = 0) out vec4 frag_color;

void main() {
  frag_color = ps_in.frag_color;
}
