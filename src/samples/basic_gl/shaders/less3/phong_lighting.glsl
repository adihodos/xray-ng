#version 440 core
#pragma debug(on)
#pragma optimize(off)

layout(row_major) uniform;

in struct VS_IN {
    layout(location = 0) vec3 vertex_position;
    layout(location = 1) vec3 vertex_normal;
} vs_in;

out struct VS_OUT {
    layout(location = 0) vec4 vertex_color;
} vs_out;

layout(binding = 0) uniform transforms {
    mat4 world_to_view;
    mat4 normal_to_view;
    mat4 world_view_projection;
};

layout(binding = 1) uniform light_settings {
    vec4 light_ka;
    vec4 light_kd;
    vec4 light_ks;
    vec3 light_eye_pos;
};

layout(binding = 2) uniform material_settings {
    vec4 mat_ka;
    vec4 mat_kd;
    vec4 mat_ks;
    float shine_factor;
};

/// \brief Reflect vector s around vector n
vec3 reflect(const in vec3 n, const in vec3 s) {
    return 2.0f * dot(n, s) * n - s;
}

void transform_to_eye(in const VS_IN vertex, out vec3 vertex_pos_eye, out vec3 normal_eye) {
    vertex_pos_eye = (world_to_view * vec4(vertex.vertex_position, 1.0f)).xyz;
    normal_eye = normalize((normal_to_view * vec4(vertex.vertex_normal, 0.0f)).xyz);
}

vec4 phong_model(in const vec3 vertex_eye, in const vec3 normal_eye) {
    vec4 vertex_color = light_ka * mat_ka;

    const vec3 s = normalize(light_eye_pos - vertex_eye);
    const float n_dot_s = max(dot(s, normal_eye), 0.0f);

    vertex_color += n_dot_s * light_kd * mat_kd;

    if (n_dot_s > 0.0f) {
        const vec3 r = reflect(normal_eye, s);
        const vec3 v = normalize(-vertex_eye);
        vertex_color += pow(max(dot(r, v), 0.0f), shine_factor) 
            * light_ks * mat_ks;
    }

    return vertex_color;
}

void main() {
    gl_Position = world_view_projection * vec4(vs_in.vertex_position, 1.0f);

    vec3 vertex_eye_pos;
    vec3 normal_eye_pos;

    transform_to_eye(vs_in, vertex_eye_pos, normal_eye_pos);
    vs_out.vertex_color = phong_model(vertex_eye_pos, normal_eye_pos);
}