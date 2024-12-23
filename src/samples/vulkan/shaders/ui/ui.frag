#version 460 core

#include "core/bindless.core.glsl"

layout (location = 1) in VS_OUT_FS_IN {
    vec4 color;
    vec2 uv;
    flat uint textureid;
} fs_in;

layout (location = 0) out vec4 FinalFragColor;

void main() {
    FinalFragColor = texture(g_Textures2DGlobal[fs_in.textureid], fs_in.uv.st) * fs_in.color;
}

