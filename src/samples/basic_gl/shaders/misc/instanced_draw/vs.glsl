#version 450 core

layout (location = 0) in vec3 vs_in_pos;
layout (location = 1) in vec3 vs_in_normal;
layout (location = 2) in vec2 vs_in_texcoord;
layout (location = 3) in uint vs_in_instance_id;

struct InstanceXData {
	mat4 wvp;
	mat4 world;
};

layout (binding = 0, row_major, std430) buffer instance_data {
  InstanceXData wvp_transform[32];
} instances;

out VS_OUT_FS_IN {
  vec3 pos;
  vec3 normal;
  vec2 texcoord;
  flat uint inst_id;
} vs_out;

out gl_PerVertex {
  vec4 gl_Position;
};

void main() {
	const InstanceXData xdata = instances.wvp_transform[vs_in_instance_id];
  gl_Position = xdata.wvp * vec4(vs_in_pos, 1.0f);

  vs_out.pos = vs_in_pos;
  vs_out.normal = normalize((xdata.world * vec4(vs_in_normal, 1.0)).xyz);
  vs_out.texcoord = vs_in_texcoord;
  vs_out.inst_id = vs_in_instance_id;
}
