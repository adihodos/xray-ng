#version 450 core

in VS_OUT_PS_IN {
    layout (location = 0) vec3 posv;
    layout (location = 1) vec3 normv;
    layout (location = 2) vec2 texc;
} ps_in;

layout (location = 0) out vec4 frag_color;

struct point_light {
    vec4 ka;
    vec4 kd;
    vec4 ks;
    vec3 posv;
};

const uint MAX_LIGHTS = 8;

layout (binding = 1) uniform point_lights {
    point_light lights[MAX_LIGHTS];
};

uniform uint lights_count;
uniform float mtl_spec_pwr;

uniform sampler2D mtl_ambient;
uniform sampler2D mtl_diffuse;
uniform sampler2D mtl_specular;

vec4 phong_lighting(
    in const vec4 mat_ka, in const vec4 mat_kd, in const vec4 mat_ks,
    in const vec3 normal, in const uint light_idx)
{
    const point_light l = lights[light_idx];

    const vec3 s = normalize(l.posv - ps_in.posv);
    const float n_dot_s = max(dot(normal, s), 0.0f);

    vec4 lit_color = mat_ka * l.ka + mat_kd * l.kd * n_dot_s;

    if (n_dot_s > 0.0f) {
        const vec3 v = normalize(-ps_in.posv);
        const vec3 r = reflect(-s, normal);
        lit_color += pow(max(dot(r, v), 0.0f), mtl_spec_pwr) * mat_ks * l.ks;
    }

    return lit_color;
}

void main() {
    const vec3 normal = normalize(ps_in.normv);
    const vec4 mka = texture(mtl_ambient, ps_in.texc);
    const vec4 mkd = texture(mtl_diffuse, ps_in.texc);
    const vec4 mks = texture(mtl_specular, ps_in.texc);

    frag_color = vec4(0);
    for (uint i = 0; i < lights_count; ++i) {
        frag_color += phong_lighting(mka, mkd, mks, normal, i);
    }
}