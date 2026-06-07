#version 460 core

#include "core/bindless.core.glsl"
#include "core/std.interface.glsl"

#if defined(__VERT_SHADER__)
layout (location = 0) in vec3 pos;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec4 color;
layout (location = 3) in vec4 tangent;
layout (location = 4) in vec2 uv;
layout (location = 5) in uint pbr;

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
    layout (location = 5) flat uint mtl_buffer_elem;
    layout (location = 6) flat uint mtl_buffer;
} BLOCKVAR;

#if defined(__VERT_SHADER__)
void main() {

#include "core/unpack.frame.glsl"
#include "core/phong.setup.glsl"

    vs_out.uv = uv;
    vs_out.mtl_buffer_elem = pbr;
    vs_out.mtl_buffer = inst.mtl_buffer;
}

#endif

#if defined(__FRAG_SHADER__)
layout (location = 0) out vec4 FinalFragColor;

#include "core/lighting.phong.glsl"

void main() {
#include "core/unpack.lights.glsl"

    const PBRMaterial mtl = g_PBRMaterialGlobal[fs_in.mtl_buffer].data[fs_in.mtl_buffer_elem];
    const vec4 diffuse = texture(g_Textures2DGlobal[mtl.base_color], fs_in.uv) * mtl.base_color_factor;
    const vec4 specular = texture(g_Textures2DGlobal[mtl.metallic], fs_in.uv) * mtl.metallic_factor;

    FinalFragColor = light_phong(
        fs_in.P, 
        fs_in.V, 
        fs_in.N, 
        vec4(0.0, 0.0, 0.0, 1.0), 
        diffuse, 
        specular, 
        dls,
        pls);
}

#endif
