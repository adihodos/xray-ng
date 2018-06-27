#version 450 core

in VS_OUT_FS_IN {
  vec3 pos_view;
  vec3 norm_view;
  vec3 texcoords;
} fs_in;

layout (location = 0) out vec4 final_frag_color;
layout (binding = 0) uniform sampler2DArray TERRAIN_COLORMAP;

void main() {
  final_frag_color = texture(TERRAIN_COLORMAP, fs_in.texcoords);
}
