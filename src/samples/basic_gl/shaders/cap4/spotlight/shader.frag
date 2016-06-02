#version 450 core
#pragma debug(on)
#pragma optimize(off)

in PS_IN {
  layout (location = 0) vec3 v_eye;
  layout (location = 1) vec3 n_eye;
} ps_in;

layout (location = 0) out vec4 frag_color;

struct spotlight {
  vec3 position;
  float cutoff_angle;
  vec3 direction;
  float power;
  vec4 ka;
  vec4 kd;
  vec4 ks;
};

const uint NUM_LIGHTS = 3;

layout (binding = 1) uniform light_sources {
  spotlight scene_lights[NUM_LIGHTS];
};

struct material {
  vec4 ke;
  vec4 ka;
  vec4 kd;
  vec4 ks;
};

layout (binding = 2) uniform object_material {
  material surface;
};

vec4 phong_lighting(in const uint idx, in const vec3 v_eye, in const vec3 n_eye) {
  const spotlight sl = scene_lights[idx];
  vec4 color = vec4(0);

  const vec3 s = normalize(sl.position - v_eye);
  const float cos_cutoff = dot(-s, sl.direction);

  if (cos_cutoff >= sl.cutoff_angle) {
    const float n_dot_s = max(dot(s, n_eye), 0.0f);
    color += n_dot_s * sl.kd * surface.kd;

    if (n_dot_s > 0) {
      const vec3 r = reflect(-s, n_eye);
      const vec3 v = normalize(-v_eye);
      color += pow(max(dot(r, v), 0.0f), surface.ks.a) *
          vec4(surface.ks.rgb, 1.0f) * sl.ks;
    }

    color *= pow(cos_cutoff, sl.power);
  }

  return color;
}

void main() {
  const vec3 n_eye = normalize(ps_in.n_eye);

  frag_color = vec4(0);
  for (uint i = 0; i < NUM_LIGHTS; ++i) {
    frag_color += phong_lighting(i, ps_in.v_eye, n_eye);
  }
}
