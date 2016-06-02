#version 450 core

in PS_IN {
  layout (location = 0) vec3 reflection_dir;
} ps_in;

layout (location = 0) out vec4 frag_color;

uniform samplerCube cube_map;

layout (binding = 1) uniform reflection_params {
  vec4 surface_color;
  float reflection_factor;
};


subroutine vec4 reflect_surface_type(void);
subroutine uniform reflect_surface_type reflect_surface_fn;

subroutine(reflect_surface_type)
vec4 reflect_skybox() {
  return texture(cube_map, ps_in.reflection_dir);
}

subroutine(reflect_surface_type)
vec4 reflect_object() {
  return mix(surface_color, texture(cube_map, ps_in.reflection_dir),
             reflection_factor);
}

void main() {
  frag_color = reflect_surface_fn();
}
