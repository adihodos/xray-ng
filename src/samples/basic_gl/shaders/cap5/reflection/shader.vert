#version 450 core

layout (row_major) uniform;

layout (location = 0) in vec3 vs_in_pos;
layout (location = 1) in vec3 vs_in_normal;

out PS_IN {
  layout (location = 0) vec3 reflection_dir;
} vs_out;

layout (binding = 0) uniform transform_pack {
  mat4 world;
  mat4 world_normal;
  mat4 world_view_projection;
  vec3 world_eye_pos;
};

subroutine vec3 compute_reflection_type(void);

subroutine uniform compute_reflection_type reflection_func;

subroutine(compute_reflection_type)
vec3 skybox_func() {
  const vec3 world_pos = (world * vec4(vs_in_pos, 1.0f)).xyz;
  return normalize(world_pos - world_eye_pos);
}

subroutine(compute_reflection_type)
vec3 reflect_surface_func() {
  const vec3 vertex_world = (world * vec4(vs_in_pos, 1.0f)).xyz;
  const vec3 surf_norm_world = normalize(
        (world_normal * vec4(vs_in_normal, 0.0f)).xyz);
  const vec3 view_dir = normalize(world_eye_pos - vertex_world);

  return normalize(reflect(-view_dir, surf_norm_world));
}

void main() {
  gl_Position = world_view_projection * vec4(vs_in_pos, 1.0f);
  vs_out.reflection_dir = reflection_func();
}
