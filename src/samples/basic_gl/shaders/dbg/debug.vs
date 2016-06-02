#version 450 core

layout (location = 0) in vec3 vs_in_pos;
layout (location = 1) in vec3 vs_in_normal;
layout (location = 2) in vec2 vs_in_texc;

out VS_OUT_GS_IN {
     vec3 pos;
     vec3 normal;
     vec2 texc;
} vs_out;

void main() {
    gl_Position = vec4(vs_in_pos, 1.0f);
    vs_out.pos = vs_in_pos;
    vs_out.normal = vs_in_normal;
    vs_out.texc = vs_in_texc;
}