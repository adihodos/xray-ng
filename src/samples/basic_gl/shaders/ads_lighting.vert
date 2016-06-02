#version 440 core
#pragma optimize(off)
#pragma debug(on)

layout(row_major) uniform;

in struct VS_IN {
    layout(location = 0) vec3 vertex_pos;
    layout(location = 1) vec3 surface_normal;
} vs_in;

out struct VS_OUT {
    layout(location = 0) vec3 vertex_color;
} vs_out;

//layout(location = 0) flat out vec3 vertex_color;

layout(binding = 0) uniform transforms {
    mat4 model_to_view;
    mat4 normal_to_view;
    mat4 world_view_proj;
};

layout(binding = 1) uniform light_settings {
    vec3 light_position;
    vec3 la;
    vec3 ld;
    vec3 ls;
};

layout(binding = 2) uniform material {
    vec3 ka;
    vec3 kd;
    vec3 ks;
    float shininess;
};

vec3 reflect(in const vec3 v, in const vec3 n) {
    return -v + 2.0f * dot(v, n) * n;
}

void main() {
    gl_Position = world_view_proj * vec4(vs_in.vertex_pos, 1.0f);
    const vec3 vertex_eye = (normal_to_view * vec4(vs_in.vertex_pos, 1.0f)).xyz;
    const vec3 n = normalize((normal_to_view * vec4(vs_in.surface_normal, 0.0f)).xyz);
    const vec3 s = normalize(light_position - vertex_eye);

    vs_out.vertex_color = la * ka;
    const float s_dot_n = max(dot(s, n), 0.0f);
    vs_out.vertex_color += s_dot_n * ld * kd;

    if (s_dot_n > 0.0f) {
        const vec3 r = normalize(reflect(s, n));
        const vec3 v = normalize(-vertex_eye);
        vs_out.vertex_color += pow(max(dot(v, r), 0.0f), shininess)
            * ls * ks;
    }
}