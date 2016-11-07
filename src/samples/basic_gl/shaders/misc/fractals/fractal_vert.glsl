#version 450 core

out gl_PerVertex {
    vec4 gl_Position;
};

layout(location = 0) in vec3 vs_in_pos;
layout(location = 1) in vec4 vs_in_color;

out VS_OUT_PS_IN {
    layout(location = 0) vec2 position;
} vs_out;

void main() {
    gl_Position = vec4(vs_in_pos, 1.0f);
    vs_out.position = vs_in_pos.xy;
}