#version 450 core

layout (location = 0) in vec3 vs_in_pos;
layout (location = 1) in vec3 vs_in_normal;
layout (location = 2) in vec2 vs_in_texcoord;

layout (binding = 0, row_major) uniform TRANSFORM_PACK {
  mat4 world_view_proj_mtx;
//  mat4 normal_mtx;
//  mat4 view_mtx;
};

layout (binding = 0) uniform sampler2D HEIGHT_MAP;

out gl_PerVertex {
  vec4 gl_Position;
};

out VS_OUT_FS_IN {
  vec3 pos_view;
  vec3 norm_view;
  vec2 texcoords;
} vs_out;

void main() {
  vec3 pos = vs_in_pos;
  pos.y = textureLod(HEIGHT_MAP, vs_in_texcoord, 0).r;
  gl_Position = world_view_proj_mtx * vec4(pos, 1.0f);
  vs_out.pos_view = pos;
  vs_out.norm_view = vs_in_normal;
  vs_out.texcoords = vs_in_texcoord;
}
