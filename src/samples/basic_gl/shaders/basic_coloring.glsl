#version 430 core

in struct VS_OUT {
    layout(location = 0) vec3 color;
} ps_in;

layout(location = 0) out vec4 frag_color;

void main() {
    frag_color = vec4(ps_in.color, 1.0f);
}