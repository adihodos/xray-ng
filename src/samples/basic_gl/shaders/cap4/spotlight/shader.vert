#version 450 core
#pragma optimize(off)
#pragma debug(on)

layout (row_major) uniform;

layout (location = 0) in vec3 vs_in_pos;
layout (location = 1) in vec3 vs_in_norm;

out PS_IN {
  layout (location = 0) vec3 v_eye;
  layout (location = 1) vec3 n_eye;
} vs_out;

layout (binding = 0) uniform obj_matrix_pack {
  mat4 tf_world_eye;
  mat4 tf_normal_eye;
  mat4 tf_wvp;
};

void main() {
  gl_Position = tf_wvp * vec4(vs_in_pos, 1.0f);
  vs_out.v_eye = (tf_world_eye * vec4(vs_in_pos, 1.0f)).xyz;
  vs_out.n_eye = (tf_normal_eye * vec4(vs_in_norm, 0.0f)).xyz;
}
