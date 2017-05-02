#version 450 core

layout (location = 0) in vec3 vs_in_pos;
layout (location = 1) in vec3 vs_in_normal;
layout (location = 2) in vec2 vs_in_texcoord;

layout (row_major, std140, binding = 0) uniform TransformMatrices {
  mat4 WORLD_VIEW_PROJECTION;
  mat4 NORMALS_TF;
  mat4 VIEW_TF;
};

out VS_OUT_PS_IN {
  vec3 view_pos;
  vec3 view_norm;
  vec2 texcoord;
} vs_out;

out gl_PerVertex {
  vec4 gl_Position;
};

void main() {
  gl_Position = WORLD_VIEW_PROJECTION * vec4(vs_in_pos, 1.0f);

  vs_out.view_pos = (VIEW_TF * vec4(vs_in_pos, 1.0f)).xyz;
  vs_out.view_norm = (NORMALS_TF * vec4(vs_in_normal, 0.0f)).xyz;
  vs_out.texcoord = vs_in_texcoord;
}