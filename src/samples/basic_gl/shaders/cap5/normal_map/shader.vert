#version 450 core

layout (row_major) uniform;

layout (location = 0) in vec3 vs_in_pos;
layout (location = 1) in vec3 vs_in_norm;
layout (location = 2) in vec3 vs_in_tangent;
layout (location = 3) in vec2 vs_in_texc;

out PS_IN {
  layout (location = 0) vec3 v_pos;
  layout (location = 1) vec3 n_eye;
  layout (location = 2) vec3 t_eye;
  layout (location = 3) vec2 texc;
} vs_out;

layout (binding = 0) uniform obj_transforms {
  mat4 world_view;
  mat4 normal_view;
  mat4 world_view_proj;
};

void main() {
  gl_Position = world_view_proj * vec4(vs_in_pos, 1.0f);

  vs_out.v_pos = vs_in_pos;
  vs_out.n_eye = vec3(normal_view * vec4(vs_in_norm, 0.0f));
  vs_out.t_eye = vec3(normal_view * vec4(vs_in_tangent, 0.0f));
  vs_out.texc = vs_in_texc;
}
