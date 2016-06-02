#version 450 core

in GS_OUT_PS_IN {
    layout (location = 0) vec4 color;
} ps_in;

layout (location = 0) out vec4 frag_color;

void main() {
    frag_color = ps_in.color;
}
