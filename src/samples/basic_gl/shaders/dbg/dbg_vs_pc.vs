#version 450 core

layout (row_major) uniform;

layout (location = 0) in vec3 vs_in_pos;
layout (location = 1) in vec4 vs_in_color;

out VS_OUT {
    layout (location = 0) vec4 color;
} vs_out;

layout (binding = 0) uniform transforms {
    mat4 model_view_projection;
};

void main() {
    gl_Position = model_view_projection * vec4(vs_in_pos, 1.0f);
    vs_out.color = vs_in_color;
}