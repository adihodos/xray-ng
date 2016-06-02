#version 450 core

in PS_IN {
  layout (location = 0) vec3 v_pos;
  layout (location = 1) vec3 n_eye;
  layout (location = 2) vec3 t_eye;
  layout (location = 3) vec2 texc;
} ps_in;

layout (location = 0) out vec4 frag_color;

struct light_source {
  vec3 pos;
  vec4 ka;
  vec4 kd;
  vec4 ks;
};

const uint NUM_LIGHTS = 1u;

layout (binding = 1) uniform scene_lights {
  light_source lights[NUM_LIGHTS];
};

uniform sampler2D diffuse_map;
uniform sampler2D normal_map;

vec4 phong_lighting(const uint idx, const vec3 light_pos, const vec3 normal, const vec4 mat) {
  const vec3 s = normalize(light_pos - ps_in.v_pos);
  const float n_dot_s = max(dot(s, normal), 0.0f);

  vec4 color = n_dot_s * lights[idx].kd * mat;


  return color;
}

void main() {
  const vec3 n_eye = normalize(ps_in.n_eye);
  const vec3 t_eye = normalize(ps_in.t_eye);
  const vec3 b_eye = cross(n_eye, t_eye);
  
  const mat3 to_tangent = mat3(t_eye.x, b_eye.x, n_eye.x,
                               t_eye.y, b_eye.y, n_eye.y,
                               t_eye.z, b_eye.z, n_eye.z);

  const vec4 surface_color = texture2D(diffuse_map, ps_in.texc);
  const vec3 surface_normal =
      2.0f * vec3(texture2D(normal_map, ps_in.texc)) - 1.0f;

  frag_color = vec4(0);
  for (uint i = 0; i < NUM_LIGHTS; ++i) {
    const vec3 light_pos_tan = to_tangent * lights[i].pos;
    frag_color += phong_lighting(i, light_pos_tan, surface_normal, surface_color);
  }
}
