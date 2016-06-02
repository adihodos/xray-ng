#version 450 core

layout (row_major) uniform;

layout (location = 0) in vec3 vs_in_pos;
layout (location = 1) in vec3 vs_in_norm;
layout (location = 2) in vec2 vs_in_texc;

out PS_IN {
  layout (location = 0) vec3 v_eye;
  layout (location = 1) vec3 n_eye;
  layout (location = 2) vec2 texc;
} vs_out;

layout (binding = 0) uniform obj_transforms {
  mat4 tf_world_view;
  mat4 tf_normal_view;
  mat4 tf_wvp;
};

void main() {
  gl_Position = tf_wvp * vec4(vs_in_pos, 1.0f);
  vs_out.v_eye = (tf_world_view * vec4(vs_in_pos, 1.0f)).xyz;
  vs_out.n_eye = (tf_normal_view * vec4(vs_in_norm, 0.0f)).xyz;
  vs_out.texc = vs_in_texc;
}
