#version 450 core

layout (row_major) uniform;

layout (location = 0) in vec3 vs_in_position;
layout (location = 1) in vec3 vs_in_normal;
layout (location = 2) in vec2 vs_in_texcoord;

out VS_OUT_PS_IN {
    layout (location = 0) vec3 vpos;
    layout (location = 1) vec3 vnormal;
    layout (location = 2) vec2 texcoord;
} vs_out;

out gl_PerVertex {
    vec4 gl_Position;
};

layout (binding = 0) uniform transform_pack {
    mat4 model_view_matrix;
    mat4 normal_view_matrix;
    mat4 model_view_proj_matrix;
};

void main() {
    gl_Position = model_view_proj_matrix * vec4(vs_in_position, 1.0f);
    vs_out.vpos = vec3(model_view_matrix * vec4(vs_in_position, 1.0f));
    vs_out.vnormal = vec3(normal_view_matrix * vec4(vs_in_normal, 0.0f));
    vs_out.texcoord = vs_in_texcoord;
}