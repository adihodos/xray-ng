#version 460 core

#include "core/bindless.core.glsl"

in VS_OUT_FS_IN {
    layout (location = 0) vec2 uv;
    layout (location = 1) flat uint mtl;
} fs_in;

layout (location = 0) out vec4 FinalFragColor;

void main() {
    const PBRMaterial mtl = g_PBRMaterialGlobal.data[fs_in.mtl];
    FinalFragColor = 
       texture(g_Textures2DGlobal[mtl.base_color], fs_in.uv) * mtl.base_color_factor;
}
