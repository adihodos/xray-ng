#version 450 core

in VS_OUT_PS_IN {
    layout (location = 0) vec3 vpos;
    layout (location = 1) vec3 vnormal;
    layout (location = 2) vec2 texcoord;
} ps_in;

layout (location = 0) out vec4 frag_color;

struct light_source_t {
    vec4 ka;
    vec4 kd;
    vec4 ks;
    vec3 pos;
};

const uint MAX_SCENE_LIGHTS = 8;
uniform uint light_count;

layout (binding = 1) uniform scene_lighting {
    light_source_t lights[MAX_SCENE_LIGHTS];
};

uniform sampler2D mat_diffuse;
uniform sampler2D mat_specular;
uniform float mat_shininess;

vec4 phong_model(
    const in vec3 pos, const in vec3 n, const uint light_idx,
    const in vec4 mat_kd, const in vec4 mat_ks)
{
    const vec3 s = normalize(lights[light_idx].pos - pos);
    const float ndots = max(dot(s, n), 0.0f);
    vec4 lit_color = lights[light_idx].ka +
        ndots * lights[light_idx].kd * mat_kd;

    if (ndots > 0.0f) {
        const vec3 v = normalize(-pos);
        const vec3 r = reflect(-s, n);
        lit_color += pow(max(dot(r, v), 0.0f), mat_shininess)
            * mat_ks * lights[light_idx].ks;
    }

    return lit_color;
}

void main() {
    frag_color = vec4(0);

    const vec4 m_kd = texture(mat_diffuse, ps_in.texcoord);
    const vec4 m_ks = texture(mat_specular, ps_in.texcoord);
    const vec3 n = normalize(ps_in.vnormal);

    for (uint idx = 0; idx < light_count; ++idx) {
        frag_color += phong_model(ps_in.vpos, n, idx, m_kd, m_ks);
    }
}
