#version 450 core
#pragma debug(on)
#pragma optimize(off)

layout (row_major) uniform;

layout (location = 0) in vec3 vs_in_pos;
layout (location = 1) in vec3 vs_in_norm;

out PS_IN {
    layout (location = 0) vec3 v_eye;
    layout (location = 1) vec3 n_eye;
} vs_out;

layout (binding = 1) uniform obj_matrix_pack {
    mat4 tf_world_view;
    mat4 tf_normals_view;
    mat4 tf_wvp;
};

// struct wave_params {
//     float wave_count;
//     float 
// }

//const float WAVES_COUNT = 3.0f;

const float TWO_PI = 6.2831853f;
const float AMPLITUDE = 1.5f;
const float PERIOD = 2 * TWO_PI;

void main() {
    vec3 new_pos = vs_in_pos;
    const float x = new_pos.x / 16;
    const float z = new_pos.z / 16;

    new_pos.y = AMPLITUDE * sin(PERIOD * (x * x + z * z));

    gl_Position = tf_wvp * vec4(new_pos, 1.0f);
    vs_out.v_eye = (tf_world_view * vec4(new_pos, 1.0f)).xyz;
    vs_out.n_eye = (tf_normals_view * vec4(vs_in_norm, 0.0f)).xyz;
}
