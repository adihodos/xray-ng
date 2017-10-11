#version 450 core

in VS_OUT_FS_IN {
  vec3 pos;
  vec3 normal;
  vec4 color;
} fs_in;

struct directional_light_t {
  vec3 dir;
  vec4 ka;
  vec4 kd;
  vec4 ks;
};

layout (binding = 1) uniform light_setup {
  directional_light_t lights[4];
};

out vec4 final_frag_color;

void main() {
  final_frag_color = vec4(0);
  vec3 n = normalize(fs_in.normal);

  for (uint i = 0; i < 4; ++i) {
    final_frag_color += lights[i].ka
        + max(dot(n, -lights[i].dir), 0.0f) * fs_in.color * lights[i].kd;
  }
}
