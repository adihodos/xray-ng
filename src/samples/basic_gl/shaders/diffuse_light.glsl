#version 440
#pragma debug(on)
#pragma optimize(off)

layout(row_major) uniform;

in struct vertex_pn_t {
    layout(location = 0) vec3 position;
    layout(location = 1) vec3 normal;
} vs_in;

out struct VS_OUT  {
    layout(location = 0) vec3 vertex_color;
} vs_out;

layout(binding = 0) uniform light_settings {
    vec3 light_pos;
    vec3 light_color;
    vec3 ambient_color;
};

layout(binding = 1) uniform transforms {
    mat4 model_to_view;
    mat4 normal_to_view;
    mat4 model_view_proj;
};

uniform vec3 surface_color;

void main() {
    gl_Position = model_view_proj * vec4(vs_in.position, 1.0f);

    const vec3 vertex_eye = (model_to_view * vec4(vs_in.position, 1.0f)).xyz;
    const vec3 s = normalize(light_pos - vertex_eye);
    const vec3 n = normalize((normal_to_view * vec4(vs_in.normal, 0.0f)).xyz);

    vs_out.vertex_color = ambient_color +
        max(dot(n, s), 0.0) * light_color * surface_color;
}
