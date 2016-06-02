#version 450 core
#pragma optimize(off)
#pragma debug(on)

in VS_OUT {
  layout(location = 0) vec4 color[2];
  layout(location = 2) vec2 texcoord;
} ps_in;

layout (location = 0) out vec4 frag_color;

const vec2 discard_treshold = vec2(0.2f, 0.2f);
const float scale_factor = 15.0f;

void main(void) {
  const bvec2 to_discard = greaterThan(
        fract(ps_in.texcoord * scale_factor),
        discard_treshold);

  if (all(to_discard))
    discard;

  frag_color = ps_in.color[int(!gl_FrontFacing)];
}
