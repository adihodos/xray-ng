#version 450 core
#pragma optimize(off)
#pragma debug(on)

layout (row_major) uniform;

layout (location = 0) in vec3 vs_in_pos;
layout (location = 1) in vec3 vs_in_norm;

out PS_IN {
  layout (location = 0) vec3 v_eye;
  layout (location = 1) vec3 v_norm;
} vs_out;

layout (binding = 0) uniform transforms {
  mat4 tf_model_view;
  mat4 tf_normal_view;
  mat4 tf_wvp;
};

void main() {
  gl_Position = tf_wvp * vec4(vs_in_pos, 1.0f);
  vs_out.v_eye = (tf_model_view * vec4(vs_in_pos, 1.0f)).xyz;
  vs_out.v_norm = (tf_normal_view * vec4(vs_in_norm, 0.0f)).xyz;
}
