#version 460 core

#include "core/bindless.core.glsl"
#include "core/std.interface.glsl"

#if defined(__VERT_SHADER__)
layout (location = 0) in vec3 pos;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec3 tangent;
layout (location = 3) in vec2 uv;

out gl_PerVertex {
    vec4 gl_Position;
};
#endif

INTERFACE_BLOCK VS_OUT_FS_IN {
    layout (location = 0) vec3 P; // surface pos in View/Eye space
    layout (location = 1) vec3 N; // normal in View/Eye space
    layout (location = 2) vec3 V; // View vector (-P) in View/Eye space
    layout (location = 3) vec2 uv;
    layout (location = 4) flat uvec3 fii; // frame id, instance buffer index, instance index
#if defined(__ADS_TEXTURED__)
    layout (location = 5) flat uint tex_amb;
    layout (location = 6) flat uint tex_diffuse;
    layout (location = 7) flat uint tex_specular;
#else
    layout (location = 5) flat uint colortex;
    layout (location = 6) flat uint texel;
#endif
} BLOCKVAR;

#if defined(__VERT_SHADER__)
void main() {

#include "core/unpack.frame.glsl"
#include "core/phong.setup.glsl"

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

#include "core/lighting.phong.glsl"

void main() {
    //
    // lights data
#include "core/unpack.lights.glsl"

#if defined(__ADS_TEXTURED__)
    const vec4 ambient = texture(g_Textures2DGlobal[fs_in.tex_amb], fs_in.uv);
    const vec4 diffuse = texture(g_Textures2DGlobal[fs_in.tex_diffuse], fs_in.uv);
    const vec4 specular = texture(g_Textures2DGlobal[fs_in.tex_specular], fs_in.uv);
#else
    const vec4 ambient = texelFetch(g_Textures1DGlobal[fs_in.colortex], int(fs_in.texel + 0), 0);
    const vec4 diffuse = texelFetch(g_Textures1DGlobal[fs_in.colortex], int(fs_in.texel + 1), 0);
    const vec4 specular = texelFetch(g_Textures1DGlobal[fs_in.colortex], int(fs_in.texel + 2), 0);
#endif

    FinalFragColor = light_phong(
            fs_in.P, 
            fs_in.V, 
            fs_in.N, 
            ambient, 
            diffuse, 
            specular, 
            dls,
            pls);
};

#endif
