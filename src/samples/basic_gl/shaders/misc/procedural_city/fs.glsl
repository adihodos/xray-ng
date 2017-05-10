#version 450 core

in VS_OUT_FS_IN {
  vec4 color;
  vec2 texcoord;
  flat int texid;
} fs_in;

layout (binding = 0) uniform sampler2DArray IMG_TEX;

out vec4 FinalFragColor;

void main() {
  FinalFragColor = texture(IMG_TEX, vec3(fs_in.texcoord, float(fs_in.texid)));
}