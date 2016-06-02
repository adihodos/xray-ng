#version 450 core
#pragma debug(on)
#pragma optimize(off)

layout(row_major) uniform;

layout(location = 0) in vec3 vs_in_position;
layout(location = 1) in vec3 vs_in_normal;
layout(location = 2) in vec2 vs_in_texcoord;

out VS_OUT {
  layout(location = 0) vec4 color[2];
  layout(location = 2) vec2 texcoord;
} vs_out;

layout(binding = 0) uniform light_source {
  vec3 light_pos;
  vec4 light_ka;
  vec4 light_kd;
  vec4 light_ks;
};

layout(binding = 1) uniform material {
  vec4 mat_ke;
  vec4 mat_ka;
  vec4 mat_kd;
  vec4 mat_ks;
};

layout(binding = 2) uniform transforms {
  mat4 tf_mv;
  mat4 tf_nv;
  mat4 tf_mvp;
};

void to_eye(out vec3 vertex_eye, out vec3 normal_eye) {
  vertex_eye = (tf_mv * vec4(vs_in_position, 1.0f)).xyz;
  normal_eye = normalize((tf_nv * vec4(vs_in_normal, 0.0f)).xyz);
}

vec4 phong_lighting(in const vec3 s, in const vec3 n, in const vec3 v_eye) {
  vec4 out_color = mat_ke + light_ka * mat_ka;
  const float n_dot_s = max(dot(s, n), 0.0f);

  out_color += n_dot_s * light_kd * mat_kd;
  if (n_dot_s > 0.0f) {
    const vec3 r = reflect(-s, n);
    const vec3 v = normalize(-v_eye);
    out_color += pow(max(dot(r, v), 0.0f), mat_ks.a) * light_ks * vec4(mat_ks.rgb, 1.0f);
  }

  return out_color;
}

void main(void) {
  gl_Position = tf_mvp * vec4(vs_in_position, 1.0f);

  vec3 v_eye;
  vec3 n_eye;
  to_eye(v_eye, n_eye);

  const vec3 s = normalize(light_pos - v_eye);
  vs_out.color[0] = phong_lighting(s, n_eye, v_eye);
  vs_out.color[1] = phong_lighting(s, -n_eye, v_eye);
  vs_out.texcoord = vs_in_texcoord;
}
