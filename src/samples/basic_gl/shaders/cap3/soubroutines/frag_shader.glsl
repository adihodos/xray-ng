#version 450 core
#pragma optimize(off)
#pragma debug(on)

in VS_OUT {
  layout(location = 0) vec4 color;
} ps_in;


layout(location = 0) out vec4 frag_color;

void main() {
  frag_color = ps_in.color;
}
