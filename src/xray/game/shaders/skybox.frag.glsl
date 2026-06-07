#version 460 core

#include "core/bindless.core.glsl"

layout (location = 0) in VS_OUT_FS_IN {
  vec3 texcoords;
  flat uint cubemap;
} fs_in;

layout (location = 0) out vec4 FinalFragColor;

void main() {
  FinalFragColor = vec4(texture(g_TexturesCubeGlobal[fs_in.cubemap], fs_in.texcoords).rgb, 1.0);
}
