#version 450 core

layout (location = 0) in vec3 vs_in_pos;
layout (location = 1) in vec3 vs_in_normal;
layout (location = 2) in vec2 vs_in_texcoord;

struct PerInstanceData {
  mat4 world;
  vec4 color;
};

layout (std430, row_major, binding = 0) buffer InstanceData {
  PerInstanceData instance_data[];
};

layout (std140, row_major, binding = 0) uniform Transforms {
  mat4 PROJ_VIEW;
};

out VS_OUT_FS_IN {
  vec4 color;
} vs_out;

out gl_PerVertex {
  vec4 gl_Position;
};

void main() {
  PerInstanceData id = instance_data[gl_InstanceID];
  gl_Position = (PROJ_VIEW * id.world) * vec4(vs_in_pos, 1.0f);
  vs_out.color = id.color;
}