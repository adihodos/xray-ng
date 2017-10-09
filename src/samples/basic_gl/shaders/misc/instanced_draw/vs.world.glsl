#version 450 core

layout (location = 0) in vec3 vs_in_pos;
layout (location = 1) in vec3 vs_in_normal;
layout (location = 2) in vec2 vs_in_texcoord;

uniform mat4 WORLD_VIEW_PROJ;
layout (binding = 0) uniform sampler2D HEIGHTMAP;

out gl_PerVertex {
  vec4 gl_Position;
};

out VS_OUT_FS_IN {
  vec3 pos;
  vec3 normal;
  vec2 texcoord;
} vs_out;

const float HEIGHT_SCALE = 10.0f;

void main() {
  vec3 pos = vec3(vs_in_pos.x, HEIGHT_SCALE * textureLod(HEIGHTMAP, vs_in_texcoord, 0.0f).g, vs_in_pos.z);
  gl_Position = WORLD_VIEW_PROJ * vec4(pos, 1.0f);
  vs_out.pos = pos;
  vs_out.normal = vs_in_normal;
  vs_out.texcoord = vs_in_texcoord;
}