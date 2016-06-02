#version 430 core

layout(row_major) uniform;

in struct VS_IN {
    layout(location = 0) vec2 position;
} vs_in;

out struct VS_OUT {
    layout(location = 0) vec3 color;
} vs_out;

uniform vec3        vertex_color;

layout(binding = 0) uniform transforms {
    mat3          rotation;
};

void main() {
    gl_Position = vec4(rotation * vec3(vs_in.position, 1.0f), 1.0f);
    vs_out.color = vertex_color;
}