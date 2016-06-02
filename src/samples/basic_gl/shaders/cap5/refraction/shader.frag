#version 450 core

in PS_IN {
  layout (location = 0) vec3 reflection_dir;
  layout (location = 1) vec3 refraction_dir;
} ps_in;

layout (location = 0) out vec4 frag_color;

uniform float surface_reflection;
uniform samplerCube environment_map;

subroutine vec4 compute_surface_color_func(void);
subroutine uniform compute_surface_color_func color_surface_func;

subroutine(compute_surface_color_func)
vec4 color_skybox() {
  return texture(environment_map, ps_in.reflection_dir);
}

subroutine(compute_surface_color_func)
vec4 color_surface() {
  const vec4 reflect_clr = texture(environment_map, ps_in.reflection_dir);
  const vec4 refract_clr = texture(environment_map, ps_in.refraction_dir);
  return mix(refract_clr, reflect_clr, surface_reflection);
}

void main() {
  frag_color = color_surface_func();
}
