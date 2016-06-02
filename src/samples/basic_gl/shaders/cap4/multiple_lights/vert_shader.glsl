#version 450 core
#pragma optimize(off)
#pragma debug(on)

layout (row_major) uniform;

layout (location = 0) in vec3 vs_in_position;
layout (location = 1) in vec3 vs_in_normal;

out VS_OUT {
  layout (location = 0) vec4 color;
} vs_out;

struct light_source {
  vec3 position;
  vec3 intensity;
  vec4 ka;
  vec4 kd;
  vec4 ks;
};

const uint NUM_LIGHTS = 5;

layout (binding = 0) uniform lighting_info {
  light_source scene_lights[NUM_LIGHTS];
};

struct material {
  vec4 ke;
  vec4 ka;
  vec4 kd;
  vec4 ks;
};

layout (binding = 1) uniform material_info {
  material surface_mat;
};

layout (binding = 2) uniform transforms {
  mat4 tf_view;
  mat4 tf_normals;
  mat4 tf_wvp;
};

void to_eye(out vec3 v_eye, out vec3 n_eye) {
  v_eye = (tf_view * vec4(vs_in_position, 1.0f)).xyz;
  n_eye = normalize((tf_normals * vec4(vs_in_normal, 0.0f)).xyz);
}

vec4 phong_model(const in uint light_idx, const in vec3 v_eye, const in vec3 n_eye) {
  const vec3 s = normalize(scene_lights[light_idx].position - v_eye);
  const float n_dot_s = max(dot(n_eye, s), 0.0f);
  const vec4 light_intensity = vec4(scene_lights[light_idx].intensity, 1.0f);

  vec4 out_color =
      light_intensity * surface_mat.ka
      * scene_lights[light_idx].ka
      + n_dot_s * surface_mat.kd * scene_lights[light_idx].kd;

  if (n_dot_s > 0.0f) {
    const vec3 v = normalize(-v_eye);
    const vec3 r = normalize(reflect(-s, n_eye));
    out_color +=
        pow(max(dot(r, v), 0.0f), surface_mat.ks.a)
        * scene_lights[light_idx].ks * vec4(surface_mat.ks.rgb, 1.0f);
  }

  out_color *= light_intensity;
  out_color += surface_mat.ke;

  return out_color;
}

void main(void) {
  gl_Position = tf_wvp * vec4(vs_in_position, 1.0f);

  vec3 v_eye;
  vec3 n_eye;

  to_eye(v_eye, n_eye);
  vs_out.color = vec4(0);

  for (uint i = 0; i < NUM_LIGHTS; ++i) {
    vs_out.color += phong_model(i, v_eye, n_eye);
  }
}


