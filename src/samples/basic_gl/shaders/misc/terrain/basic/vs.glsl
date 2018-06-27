#version 450 core

layout (location = 0) in vec3 vs_in_pos;
layout (location = 1) in vec3 vs_in_normal;
layout (location = 2) in vec2 vs_in_texcoord;

layout (binding = 0, std430) buffer buff_instance_heightmaps {
  float instance_vertices[];
};

struct per_instance_transform {
  mat4 world;
  mat4 world_view_projection;
  uint instance_vertex_offset;
};

layout (binding = 1, std430) buffer buff_instance_transforms {
  per_instance_transform instance_transforms[];
};

out gl_PerVertex {
  vec4 gl_Position;
};

out VS_OUT_FS_IN {
  vec3 pos_view;
  vec3 norm_view;
  vec3 texcoords;
} vs_out;

void main() {
  vec3 pos = vs_in_pos;
  pos.y =  instance_vertices[instance_transforms[gl_InstanceID].instance_vertex_offset + gl_VertexID];
  gl_Position = instance_transforms[gl_InstanceID].world_view_projection * vec4(pos, 1.0f);
  vs_out.pos_view = pos;
  vs_out.norm_view = vs_in_normal;
  vs_out.texcoords = vec3(vs_in_texcoord, gl_InstanceID);
}
