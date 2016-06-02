#version 450 core
#pragma optimize(off)
#pragma debug(on)

in PS_IN {
  layout (location = 0) vec3 v_eye;
  layout (location = 1) vec3 v_norm;
} ps_in;

layout (location = 0) out vec4 frag_color;

struct light_source {
  vec3 position;
  vec4 intensity;
};

const uint NUM_LIGHTS = 1;

layout (binding = 1) uniform light_source_pack {
  light_source scene_lights[NUM_LIGHTS];
};

struct material {
  vec4 ka;
  vec4 kd;
};

layout (binding = 2) uniform object_material {
  material surface;
};

const uint NUM_LEVELS = 3;
const float LEVEL_LIGHT = 1.0f / NUM_LEVELS;

vec4 toon_shade(uint idx, const vec3 v_eye, const vec3 n_eye) {
  const light_source l = scene_lights[idx];
  const vec3 s = normalize(l.position - v_eye);
  const float cosine = max(dot(s, n_eye), 0.0f);
  const vec4 diffuse = surface.kd * floor(cosine * NUM_LEVELS) * LEVEL_LIGHT;

  return l.intensity * (surface.ka + diffuse);
}

void main() {
  frag_color = vec4(0);

  const vec3 n_eye = normalize(ps_in.v_norm);

  for (uint i = 0; i < NUM_LIGHTS; ++i) {
    frag_color += toon_shade(i, ps_in.v_eye, n_eye);
  }
}
