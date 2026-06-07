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

