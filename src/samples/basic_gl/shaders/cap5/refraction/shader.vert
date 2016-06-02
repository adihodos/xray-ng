#version 450 core

layout (row_major) uniform;

layout (location = 0) in vec3 vs_in_pos;
layout (location = 1) in vec3 vs_in_normal;

out PS_IN {
  layout (location = 0) vec3 reflection_dir;
  layout (location = 1) vec3 refraction_dir;
} vs_out;

layout (binding = 0) uniform transforms_pack {
  mat4 model_world;
  mat4 normal_world;
  mat4 world_view_proj;
};

uniform float material_eta;
uniform vec3 eye_pos;

subroutine void compute_reflection_refraction_func(void);
subroutine uniform compute_reflection_refraction_func surface_process_func;

subroutine(compute_reflection_refraction_func)
void process_skybox() {
  vs_out.reflection_dir = normalize(
        vec3(model_world * vec4(vs_in_pos, 1.0f)) - eye_pos);
}

subroutine(compute_reflection_refraction_func)
void process_surface() {
  const vec3 pos_world = (model_world * vec4(vs_in_pos, 1.0f)).xyz;
  const vec3 ray_dir = normalize(pos_world - eye_pos);
  const vec3 normal_world =
      normalize((normal_world * vec4(vs_in_normal, 0.0f)).xyz);

  vs_out.reflection_dir = reflect(ray_dir, normal_world);
  vs_out.refraction_dir = refract(ray_dir, normal_world, material_eta);
}

void main() {
  gl_Position = world_view_proj * vec4(vs_in_pos, 1.0f);
  surface_process_func();
}
