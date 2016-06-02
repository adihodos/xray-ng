#version 440
#pragma debug(on)
#pragma optimize(off)

in struct VS_OUT {
    layout(location = 0) vec3 vertex_color;
} ps_in;

layout(location = 0) out vec4 frag_color;

void main() {
    if (gl_FrontFacing)
        frag_color = vec4(ps_in.vertex_color, 1.0f);
    else
        frag_color = vec4(mix(ps_in.vertex_color, vec3(1.0f, 0.0f, 0.0f), 0.7f), 1.0f);
}