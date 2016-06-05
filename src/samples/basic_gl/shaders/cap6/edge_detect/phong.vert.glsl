#version 450 core

layout (row_major) uniform;

layout (location = 0) in vec3 vs_in_pos;
layout (location = 1) in vec3 vs_in_normal;
layout (location = 2) in vec2 vs_in_texc;

out VS_OUT_PS_IN {
    layout (location = 0) vec3 posv;
    layout (location = 1) vec3 normv;
    layout (location = 2) vec2 texc;
} vs_out;

layout (binding = 0) uniform object_transforms {
    mat4 to_view;
    mat4 to_view_normals;
    mat4 to_clipspace;
};

void main() {
    gl_Position = to_clipspace * vec4(vs_in_pos, 1.0f);

    vs_out.posv = vec3(to_view * vec4(vs_in_pos, 1.0f));
    vs_out.normv = vec3(to_view_normals * vec4(vs_in_normal, 0.0f));
    vs_out.texc = vs_in_texc;
}