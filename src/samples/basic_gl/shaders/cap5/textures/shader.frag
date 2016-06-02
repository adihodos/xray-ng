#version 450 core
#pragma optimize(off)
#pragma debug(on)

in PS_IN {
  layout (location = 0) vec3 v_eye;
  layout (location = 1) vec3 n_eye;
  layout (location = 2) vec2 texcoord;
} ps_in;

layout (location = 0) out vec4 frag_color;

uniform sampler2D material;

void main() {
  frag_color = texture2D(material, ps_in.texcoord);
}
