layout (location = 0) in vec3 vs_in_pos;
layout (location = 1) in vec3 vs_in_norm;
layout (location = 2) in vec4 vs_in_color;

#if defined(NO_INSTANCING)

layout (binding = 0, row_major) uniform Transforms {
  mat4 WORLD_VIEW_PROJ;
  mat4 WORLD_VIEW;
  mat4 NORMALS;
};

#else /* defined NO_INSTANCING */

struct gpu_instance_info {
  mat4 world_view_proj;
  mat4 world_view;
  mat4 normals;
  vec4 color;
  uint tex_id;
};

layout (binding = 0, std430) buffer per_instance_data {
  gpu_instance_info data[64];
} instances;

#endif

out VS_OUT_FS_IN {
  vec3 pos;
  vec3 normal;
  vec4 color;
} vs_out;

out gl_PerVertex {
  vec4 gl_Position;
};

void main() {
#if defined(NO_INSTANCING)

  gl_Position = WORLD_VIEW_PROJ * vec4(vs_in_pos, 1.0f);
  vs_out.pos = (WORLD_VIEW * vec4(vs_in_pos, 1.0f)).xyz;
  vs_out.normal = (NORMALS * vec4(vs_in_norm, 0.0f)).xyz;
  vs_out.color = vs_in_color;

#else /* defined NO_INSTANCING */

  gl_Position = instances.data[gl_InstanceID].world_view_proj * vec4(vs_in_pos, 1.0f);
  vs_out.pos = (instances.data[gl_InstanceID].world_view * vec4(vs_in_pos, 1.0f)).xyz;
  vs_out.normal = (instances.data[gl_InstanceID].normals * vec4(vs_in_norm, 0.0f)).xyz;
  vs_out.color = instances.data[gl_InstanceID].color;

#endif


}
