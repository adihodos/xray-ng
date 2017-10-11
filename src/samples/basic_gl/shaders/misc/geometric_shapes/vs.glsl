#version 450 core

layout (location = 0) in vec3 vs_in_pos;
layout (location = 1) in vec3 vs_in_norm;
layout (location = 2) in vec4 vs_in_color;

layout (binding = 0, row_major) uniform Transforms {
  mat4 WORLD_VIEW_PROJ;
  mat4 VIEW_MATRIX;
  mat4 NORMAL_MATRIX;
};

out VS_OUT_FS_IN {
  vec3 pos;
  vec3 normal;
  vec4 color;
} vs_out;

out gl_PerVertex {
  vec4 gl_Position;
};

void main() {
  gl_Position = WORLD_VIEW_PROJ * vec4(vs_in_pos, 1.0f);
  vs_out.pos = (VIEW_MATRIX * vec4(vs_in_pos, 1.0f)).xyz;
  vs_out.normal = (NORMAL_MATRIX * vec4(vs_in_norm, 0.0f)).xyz;
  vs_out.color = vs_in_color;
}
