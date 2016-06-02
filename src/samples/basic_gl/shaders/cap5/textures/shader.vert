#version 450 core
#pragma debug(on)
#pragma optimize(off)

layout (row_major) uniform;

layout (location = 0) in vec3 vs_in_position;
layout (location = 1) in vec3 vs_in_normal;
layout (location = 2) in vec2 vs_in_texcoord;

out PS_IN {
  layout (location = 0) vec3 v_eye;
  layout (location = 1) vec3 n_eye;
  layout (location = 2) vec2 texcoord;
} vs_out;

layout (binding = 0) uniform matrix_pack {
  mat4 tf_world_view;
  mat4 tf_normals_view;
  mat4 tf_wvp;
};

void main() {
  gl_Position = tf_wvp * vec4(vs_in_position, 1.0f);
  vs_out.v_eye = (tf_world_view * vec4(vs_in_position, 1.0f)).xyz;
  vs_out.n_eye = (tf_normals_view * vec4(vs_in_normal, 0.0f)).xyz;
  vs_out.texcoord = vs_in_texcoord;
}
