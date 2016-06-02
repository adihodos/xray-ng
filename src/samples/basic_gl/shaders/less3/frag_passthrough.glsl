#version 440 core
// #pragma debug(on)
// #pragma optimize(off)
// 
in struct VS_OUT {
    vec4 vertex_color;
} ps_in;

layout(location = 0) out vec4 frag_color;

void main() {
    frag_color = ps_in.vertex_color;
}