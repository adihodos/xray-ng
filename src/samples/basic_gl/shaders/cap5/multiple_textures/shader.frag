#version 450 core

in PS_IN {
  layout (location = 0) vec3 v_eye;
  layout (location = 1) vec3 n_eye;
  layout (location = 2) vec2 texc;
} ps_in;

layout (location = 0) out vec4 frag_color;

uniform sampler2D base_mtl;
uniform sampler2D overlay_mtl;

void main() {
  const vec4 brick_color = texture2D(base_mtl, ps_in.texc);
  const vec4 moss_color = texture2D(overlay_mtl, ps_in.texc);

  frag_color = mix(brick_color, moss_color, moss_color.a);
}
