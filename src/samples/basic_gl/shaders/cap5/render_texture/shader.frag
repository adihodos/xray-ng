#version 450 core

in PS_IN {
  layout (location = 0) vec3 pos;
  layout (location = 1) vec3 normal;
  layout (location = 2) vec2 texc;
} ps_in;

layout (location = 0) out vec4 frag_color;

struct light_source_t {
  vec4 kd;
  vec3 pos;
};

uniform sampler2D surface_mtl;

const uint MAX_LIGHTS = 2;

layout (binding = 1) uniform scene_light_sources {
  light_source_t lights[MAX_LIGHTS];
};

vec4 phong_diffuse(
  const in uint lidx, 
  const in vec3 pos, 
  const in vec3 n,
  const in vec4 surface_diffuse) 
{
  const vec3 s = normalize(lights[lidx].pos - pos);
  const float n_dot_s = max(dot(n, s), 0.0f);
  return n_dot_s * lights[lidx].kd * surface_diffuse;
}

void main() {
  const vec4 surface_color = texture(surface_mtl, ps_in.texc);
  frag_color = vec4(0);
  const vec3 n = normalize(ps_in.normal);

  for (uint idx = 0; idx < MAX_LIGHTS; ++idx) {
    frag_color += phong_diffuse(idx, ps_in.pos, n, surface_color);
  }
}
