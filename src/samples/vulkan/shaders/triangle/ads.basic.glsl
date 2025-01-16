#version 460 core

#include "core/bindless.core.glsl"

#if defined(__VERT_SHADER__)
#define INTERFACE_BLOCK out
#define BLOCKVAR vs_out
#elif defined(__FRAG_SHADER__)
#define INTERFACE_BLOCK in
#define BLOCKVAR fs_in
#else
#error "Unkown shader kind"
#endif

#if defined(__VERT_SHADER__)
layout (location = 0) in vec3 pos;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec3 tangent;
layout (location = 3) in vec2 texcoords;

out gl_PerVertex {
    vec4 gl_Position;
};
#endif

INTERFACE_BLOCK VS_OUT_FS_IN {
    layout (location = 0) vec2 uv;
#if defined(__ADS_TEXTURED__)
    layout (location = 1) flat uint tex_amb;
    layout (location = 2) flat uint tex_diffuse;
    layout (location = 3) flat uint tex_specular;
#else
    layout (location = 1) flat uint colortex;
    layout (location = 2) flat uint texel;
#endif
} BLOCKVAR;

#if defined(__VERT_SHADER__)
void main() {
    const uint frame_idx = (g_GlobalPushConst.data) & 0xFF;
    const uint inst_buffer_idx = (g_GlobalPushConst.data & 0xFFFF0000) >> 16;
    const uint instance_index = (g_GlobalPushConst.data & 0x0000FF00) >> 8;
    const FrameGlobalData_t fgd = g_FrameGlobal[frame_idx].data[0];

    const InstanceRenderInfo_t inst = g_InstanceGlobal[inst_buffer_idx].data[instance_index];
    gl_Position = fgd.world_view_proj * inst.model * vec4(pos, 1.0);
    vs_out.uv = texcoords;

#if defined(__ADS_TEXTURED__)
    const MaterialADSTextured mtl = g_MaterialADSTexturedGlobal[inst.mtl_buffer].data[inst.mtl_buffer_elem];
    vs_out.tex_amb = mtl.ambient;
    vs_out.tex_diffuse = mtl.diffuse;
    vs_out.tex_specular = mtl.specular;
#else
    vs_out.colortex = fgd.global_color_texture;
    vs_out.texel = g_MaterialADSColoredGlobal[inst.mtl_buffer].data[inst.mtl_buffer_elem].texel_offset;
#endif
}

#endif

#if defined(__FRAG_SHADER__)
layout (location = 0) out vec4 FinalFragColor;

void main() {
#if defined(__ADS_TEXTURED__)
    const vec4 ambient = texture(g_Textures2DGlobal[fs_in.tex_amb], fs_in.uv);
    const vec4 diffuse = texture(g_Textures2DGlobal[fs_in.tex_diffuse], fs_in.uv);
    const vec4 specular = texture(g_Textures2DGlobal[fs_in.tex_specular], fs_in.uv);
#else
    const vec4 ambient = texelFetch(g_Textures1DGlobal[fs_in.colortex], int(fs_in.texel + 0), 0);
    const vec4 diffuse = texelFetch(g_Textures1DGlobal[fs_in.colortex], int(fs_in.texel + 1), 0);
    const vec4 specular = texelFetch(g_Textures1DGlobal[fs_in.colortex], int(fs_in.texel + 2), 0);
#endif

    FinalFragColor = ambient * 0.10 + diffuse * 0.85 + specular * 0.5;
};

#endif
