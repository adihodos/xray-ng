#version 450 core

layout (row_major) uniform;

layout (location = 0) in vec3 vs_in_pos;
layout (location = 1) in vec3 vs_in_norm;

out PS_IN {
    layout (location = 0) vec3 pos;
    layout (location = 1) vec3 normal;
} vs_out;

layout (binding = 0) uniform obj_transforms {
    mat4 tf_normals;
    mat4 tf_wvp;
};

void main() {
    gl_Position = tf_wvp * vec4(vs_in_pos, 1.0f);
    vs_out.pos = vs_in_pos;
    vs_out.normal = (tf_normals * vec4(vs_in_norm, 0.0f)).xyz;
}
