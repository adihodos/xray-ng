#version 450 core
#pragma debug(on)
#pragma optimize(off)

in PS_IN {
  layout (location = 0) vec3 v_eye;
  layout (location = 1) vec3 v_norm;
} ps_in;

layout (location = 0) out vec4 frag_color;

struct directional_light {
  vec3 direction;
  vec4 ka;
  vec4 kd;
  vec4 ks;
};

const uint NUM_LIGHTS = 4;

layout (binding = 1) uniform scene_lights {
  directional_light lights[NUM_LIGHTS];
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

vec4 phong_model(in uint idx, in const vec3 v_eye, in const vec3 n_eye) {
  const directional_light l = lights[idx];

  const vec3 s = -l.direction;
  const float n_dot_s = max(dot(s, n_eye), 0.0f);
  vec4 color = surface.ke + l.ka * surface.ka + n_dot_s * l.kd * surface.kd;

  if (n_dot_s > 0.0f) {
    const vec3 r = normalize(reflect(l.direction, n_eye));
    const vec3 v = normalize(-v_eye);
    color += pow(max(dot(r, v), 0.0f), surface.ks.a)
        * vec4(surface.ks.rgb, 1.0f) * l.ks;
  }

  return color;
}

void main() {
  frag_color = vec4(0);

  const vec3 n_eye = normalize(ps_in.v_norm);
  for (uint i = 0; i < NUM_LIGHTS; ++i) {
    frag_color += phong_model(i, ps_in.v_eye, n_eye);
  }
}
