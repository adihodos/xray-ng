#version 450 core

layout (location = 0) in vec3 vs_in_pos;
layout (location = 1) in vec3 vs_in_normal;
layout (location = 2) in vec2 vs_in_texcoord;
layout (location = 3) in mat4 vs_inst_wvp;
layout (location = 4) in uint vs_inst_texid;

out VS_OUT_FS_IN {
  vec3 pos;
  vec3 normal;
  vec2 texcoord;
  uint texid;
} vs_out;

out gl_PerVertex {
  vec4 gl_Position;
};

void main() {
  gl_Position = vs_inst_wvp * vec4(vs_in_pos, 1.0f);

  vs_out.pos = vs_in_pos;
  vs_out.normal = vs_in_normal;
  vs_out.texcoord = vs_in_texcoord;
  vs_out.texid = vs_inst_texid;
}
