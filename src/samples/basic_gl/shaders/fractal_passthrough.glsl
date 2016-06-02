#version 440 core

in struct vertex_p {
    layout(location = 0) vec3 position;
    layout(location = 1) vec4 color;
} vs_in;

out struct vertex_pc {
    layout(location = 0) vec2 position;
    layout(location = 1) vec4 color;
} ps_in;

void main() {
    gl_Position = vec4(vs_in.position, 1.0f);
    ps_in.position = vs_in.position.xy;
    ps_in.color = vs_in.color;
}