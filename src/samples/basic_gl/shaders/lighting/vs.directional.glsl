#version 450 core

layout (location = 0) in vec3 vs_in_pos;
layout (location = 1) in vec3 vs_in_normal;
layout (location = 2) in vec2 vs_in_texc;

layout (row_major, binding = 0) uniform TransformMatrices {
  mat4 WORLD_VIEW_PROJ;
  mat4 VIEW;
  mat4 NORMALS;
};

out gl_PerVertex {
  vec4 gl_Position;
};

out VS_OUT_FS_IN {
  vec3 pos_view;
  vec3 norm_view;
  vec2 texcoord;
} vs_out;

void main() {
  gl_Position = WORLD_VIEW_PROJ * vec4(vs_in_pos, 1.0f);
  vs_out.pos_view = (VIEW * vec4(vs_in_pos, 1.0f)).xyz;
  vs_out.norm_view = (NORMALS * vec4(vs_in_normal, 0.0f)).xyz;
  vs_out.texcoord = vs_in_texc;
}