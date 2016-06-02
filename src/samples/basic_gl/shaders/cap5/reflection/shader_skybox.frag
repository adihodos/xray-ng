#version 450 core

in PS_IN {
    layout (location = 0) vec3 pos;
    layout (location = 1) vec3 normal;
} ps_in;

layout (location = 0) out vec4 frag_color;

void main() {
    frag_color = vec4(normalize(ps_in.normal), 1.0f);
}
