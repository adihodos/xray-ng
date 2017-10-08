#version 450 core

layout (binding = 1, std430) buffer instance_data {
  uint texture_id[32];
} instances;

in VS_OUT_FS_IN {
  vec3 pos;
  vec3 normal;
  vec2 texcoord;
  flat uint inst_id;
} fs_in;

layout (location = 0) out vec4 final_frag_color;

layout (binding = 0) uniform sampler2DArray TEXTURES;

void main() {
  final_frag_color = texture(TEXTURES, vec3(fs_in.texcoord, float(fs_in.inst_id)));

      //vec4(vec3(float(instances.texture_id[fs_in.inst_id]) / 8.0f), 1.0f);
}
