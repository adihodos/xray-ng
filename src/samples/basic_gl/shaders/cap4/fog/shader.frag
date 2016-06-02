#version 450 core
#pragma optimize(off)
#pragma debug(on)

in PS_IN {
  layout (location = 0) vec3 v_eye;
  layout (location = 1) vec3 n_eye;
} ps_in;

layout (location = 0) out vec4 frag_color;

struct light_source {
  vec3 position;
  vec4 ka;
  vec4 kd;
  vec4 ks;
};

layout (binding = 1) uniform scene_lights {
  light_source light;
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

struct fog_params {
  vec4  color;
  float d_min;
  float d_max;
  float density;
};

layout (binding = 3) uniform scene_fog {
  fog_params fog;
};

subroutine float fog_factor_type(const vec3 v_eye);

subroutine(fog_factor_type)
float fog_linear(const vec3 v_eye) {
  const float dist_to_eye = length(v_eye);
  return (fog.d_max - dist_to_eye) / (fog.d_max - fog.d_min);
}

subroutine(fog_factor_type)
float fog_exponential(const vec3 v_eye) {
  const float dist_to_eye = length(v_eye);
  const float E = 2.71828f;
  return pow(E, -fog.density * dist_to_eye);
}

subroutine uniform fog_factor_type fog_type;

vec4 phong_lighting(const vec3 v_eye, const vec3 n_eye) {
  vec4 color = surface.ke + surface.ka * light.ka;

  const vec3 s = normalize(light.position - v_eye);
  const float n_dot_s = max(dot(s, n_eye), 0.0f);

  color += n_dot_s * surface.kd * light.kd;

  return color;
}


void main() {
  const vec3 n_eye = normalize(ps_in.n_eye);
  const vec4 clr = phong_lighting(ps_in.v_eye, n_eye);
  const float fog_factor = clamp(fog_type(ps_in.v_eye), 0.0f, 1.0f);

  frag_color = mix(fog.color, clr, fog_factor);
}
