#version 450 core
#pragma optimize(off)
#pragma debug(on)

layout (row_major) uniform;

layout (location = 0) in vec3 vs_in_position;
layout (location = 1) in vec3 vs_in_normal;

out PS_IN {
  layout (location = 0) vec4 frag_color;
} vs_out;

struct directional_light {
  vec3 direction;
  vec4 ka;
  vec4 kd;
  vec4 ks;
};

const uint NUM_LIGHTS = 4;

layout (binding = 0) uniform scene_lights {
  directional_light lights[NUM_LIGHTS];
};

struct material {
  vec4 ke;
  vec4 ka;
  vec4 kd;
  vec4 ks;
};

layout (binding = 1) uniform obj_mat {
  material surface;
};

layout (binding = 2) uniform transforms {
  mat4 tf_world_view;
  mat4 tf_normal_view;
  mat4 tf_wvp;
};

void to_eye(out vec3 v_eye, out vec3 n_eye) {
  v_eye = (tf_world_view * vec4(vs_in_position, 1.0f)).xyz;
  n_eye = normalize((tf_normal_view * vec4(vs_in_normal, 0.0f)).xyz);
}

vec4 phong_lighting(in const uint idx, in const vec3 v_eye, in const vec3 n_eye) {
  const directional_light l = lights[idx];

  vec4 out_color = surface.ke + l.ka * surface.ka;

  const vec3 s = -l.direction;
  const float n_dot_s = max(dot(n_eye, s), 0.0f);
  out_color += n_dot_s * l.kd * surface.kd;

  if (n_dot_s > 0.0f) {
    const vec3 v = normalize(-v_eye);
    const vec3 r = reflect(l.direction, n_eye);
    out_color += pow(max(dot(r, v), 0.0f), surface.ks.a)
        * vec4(surface.ks.rgb, 1.0f) * l.ks;
  }

  return out_color;
}

void main(void) {
  gl_Position = tf_wvp * vec4(vs_in_position, 1.0f);

  vec3 v_eye;
  vec3 n_eye;
  to_eye(v_eye, n_eye);

  vs_out.frag_color = vec4(0);

  for (uint i = 0; i < NUM_LIGHTS; ++i) {
    vs_out.frag_color += phong_lighting(i, v_eye, n_eye);
  }
}
