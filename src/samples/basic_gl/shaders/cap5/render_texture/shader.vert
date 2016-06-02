#version 450 core

layout (row_major) uniform;

layout (location = 0) in vec3 vs_in_pos;
layout (location = 1) in vec3 vs_in_norm;
layout (location = 2) in vec2 vs_in_texc;

out PS_IN {
  layout (location = 0) vec3 pos;
  layout (location = 1) vec3 normal;
  layout (location = 2) vec2 texc;
} vs_out;

layout (binding = 0) uniform transform_pack {
  mat4 model_view;
  mat4 normal_view;
  mat4 model_view_proj;
};

void main() {
  gl_Position = model_view_proj * vec4(vs_in_pos, 1.0f);
  vs_out.pos = vec3(model_view * vec4(vs_in_pos, 1.0f));
  vs_out.normal = vec3(normal_view * vec4(vs_in_norm, 0.0f));
  vs_out.texc = vs_in_texc;
}
