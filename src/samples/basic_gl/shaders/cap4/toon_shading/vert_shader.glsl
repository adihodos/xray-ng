#version 450 core
#pragma optimize(off)
#pragma debug(on)

layout (row_major) uniform;

layout (location = 0) in vec3 vs_in_position;
layout (location = 1) in vec3 vs_in_normal;

out PS_IN {
  layout (location = 0) vec3 v_eye;
  layout (location = 1) vec3 v_norm;
} vs_out;

layout (binding = 0) uniform transform_matrix_pack {
  mat4 tf_world_view;
  mat4 tf_norm_view;
  mat4 tf_wvp;
};

void main() {
  gl_Position = tf_wvp * vec4(vs_in_position, 1.0f);
  vs_out.v_eye = (tf_world_view * vec4(vs_in_position, 1.0f)).xyz;
  vs_out.v_norm = normalize((tf_norm_view * vec4(vs_in_normal, 0.0f)).xyz);
}
