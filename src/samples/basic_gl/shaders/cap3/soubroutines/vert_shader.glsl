#version 450 core
#pragma debug(on)
#pragma optimize(off)

layout(row_major) uniform;

layout(location = 0) in vec3 vs_in_position;
layout(location = 1) in vec3 vs_in_normal;

out VS_OUT {
  layout(location = 0) vec4 color;
} vs_out;

layout(binding = 0) uniform transforms {
  mat4 world_view_projection;
  mat4 model_view;
  mat4 normal_view;
};

layout(binding = 1) uniform light_info {
  vec3 light_eye_pos;
  vec4 light_ka;
  vec4 light_kd;
  vec4 light_ks;
};

layout(binding = 2) uniform material_info {
  vec4 mat_ke;
  vec4 mat_ka;
  vec4 mat_kd;
  vec4 mat_ks;
};

subroutine vec4 shading_model_type(const vec3 vertex_pos_eye, const vec3 normal_eye);

subroutine uniform shading_model_type shading_model;

subroutine(shading_model_type)
vec4 light_model_diffuse(const vec3 vertex_pos_eye, const vec3 normal_eye) {

  vec4 out_color = mat_ke + light_ka * mat_ka;
  const vec3 s = normalize(light_eye_pos - vertex_pos_eye);

  out_color += max(dot(normal_eye, s), 0.0f) * light_kd * mat_kd;
  return out_color;
}

subroutine(shading_model_type)
vec4 light_model_ads(const vec3 vertex_eye, const vec3 normal_eye) {

  vec4 out_color = mat_ke + light_ka * mat_ka;
  const vec3 s = normalize(light_eye_pos - vertex_eye);
  const float n_dot_s = max(dot(normal_eye, s), 0.0f);

  if (n_dot_s > 0) {
    out_color += n_dot_s * mat_kd * light_kd;
    const vec3 r = normalize(reflect(-s, normal_eye));
    const vec3 v = normalize(-vertex_eye);
    out_color += pow(max(dot(r, v), 0.0f), mat_ks.a) * light_ks * mat_ks;
  }

  return out_color;

//  vec4 vertex_color = mat_ke + light_ka * mat_ka;

//  const vec3 s = normalize(light_eye_pos - vertex_eye);
//  const float n_dot_s = max(dot(s, normal_eye), 0.0f);

//  vertex_color += n_dot_s * light_kd * mat_kd;

//  if (n_dot_s > 0.0f) {
//      const vec3 r = reflect(-s, normal_eye);
//      const vec3 v = normalize(-vertex_eye);
//      vertex_color += pow(max(dot(r, v), 0.0f), mat_ks.a)
//          * light_ks * mat_ks;
//  }

//  return vertex_color;

}

void to_eye(out vec3 vert_eye, out vec3 normal_eye) {
  vert_eye = (model_view * vec4(vs_in_position, 1.0f)).xyz;
  normal_eye = normalize((normal_view * vec4(vs_in_normal, 0.0f)).xyz);
}

void main() {
  gl_Position = world_view_projection * vec4(vs_in_position, 1.0f);
  vec3 v_eye;
  vec3 n_eye;

  to_eye(v_eye, n_eye);
  vs_out.color = shading_model(v_eye, n_eye);
}
