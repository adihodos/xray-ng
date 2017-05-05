#version 450 core

layout (binding = 0) uniform sampler2D DIFFUSE_MAP;

in VS_OUT_FS_IN {
  vec3 pos_view;
  vec3 norm_view;
  vec2 texcoord;
} fs_in;

layout (location = 0) out vec4 FinalFragColor;

void main() {
  FinalFragColor = texture(DIFFUSE_MAP, fs_in.texcoord);
}