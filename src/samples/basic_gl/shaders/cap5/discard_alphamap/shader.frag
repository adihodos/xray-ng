#version 450 core

in PS_IN {
  layout (location = 0) vec3 v_eye;
  layout (location = 1) vec3 n_eye;
  layout (location = 2) vec2 texc;
} ps_in;

layout (location = 0) out vec4 frag_color;

uniform sampler2D obj_material;
uniform sampler2D discard_map;

struct spotlight {
  vec3 pos;
  float cuttof_angl;
  vec3 dir;
  float power;
  vec4 ka;
  vec4 kd;
  vec4 ks;
};

const uint NUM_LIGHTS = 2u;

layout (binding = 1) uniform scene_lights {
  spotlight lights[NUM_LIGHTS];
};

vec4 phong_lighting(
    const uint idx, const vec3 v_eye, const vec3 n_eye, const vec4 mtl) {

  const spotlight sl = lights[idx];

  vec4 frag_color = vec4(0);
  const vec3 s = normalize(sl.pos - v_eye);
  const float cos_angle = dot(-s, sl.dir);

  if (cos_angle > sl.cuttof_angl) {
    const float n_dot_s = max(dot(n_eye, s), 0.0f);
    frag_color += n_dot_s * (sl.kd * mtl);
  }

  frag_color *= pow(cos_angle, sl.power);
  return frag_color;
}

void main() {
  const vec4 dc = texture2D(discard_map, ps_in.texc);
  if (dc.a < 0.15f)
    discard;

  const float normal_corrector[2] = {-1.0f, +1.0f};

  frag_color = vec4(0);
  const vec3 n_eye = normalize(ps_in.n_eye);
  const vec4 surface_mtl = texture2D(obj_material, ps_in.texc);

  for (uint i = 0; i < NUM_LIGHTS; ++i) {
    frag_color += phong_lighting(
          i, ps_in.v_eye, n_eye * normal_corrector[int(gl_FrontFacing)], surface_mtl);
  }
}
