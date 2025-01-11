#version 460 core

#include "core/bindless.core.glsl"

out VS_OUT_FS_IN {
    layout (location = 0) vec2 uv;
    layout (location = 1) flat uint colortex;
    layout (location = 2) flat uint texel;
} fs_in;

layout (location = 0) out vec4 FinalFragColor;

void main() {
    const vec4 ambient = texelFetch(g_Textures1DGlobal[fs_in.colortex], fs_in.texel + 0, 0);
    const vec4 diffuse = texelFetch(g_Textures1DGlobal[fs_in.colortex], fs_in.texel + 1, 0);
    const vec4 specular = texelFetch(g_Textures1DGlobal[fs_in.colortex], fs_in.texel + 2, 0);

    FinalFragColor = ambient * 0.10 + diffuse * 0.85 + specular * 0.5;
};
