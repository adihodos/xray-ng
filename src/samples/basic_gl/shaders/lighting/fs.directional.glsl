#version 450 core

layout (binding = 0) uniform sampler2D DIFFUSE_MAP;

struct DirectionalLight {
  vec4 ka;
  vec4 kd;
  vec4 ks;
  vec3 direction;
};

layout (binding = 1) uniform LightData {
  DirectionalLight SceneLights[8];
  uint NumLights;
  float SpecularIntensity;
};

in VS_OUT_FS_IN {
  vec3 pos_view;
  vec3 norm_view;
  vec2 texcoord;
} fs_in;

layout (location = 0) out vec4 FinalFragColor;

void main() {
  vec4 clr = vec4(0);

  for (uint i = 0; i < NumLights; ++i) {
    clr += SceneLights[i].ka;

    const vec3 surface_normal = normalize(fs_in.norm_view);
    const float diff_intensity = max(
      dot(-SceneLights[i].direction, surface_normal), 0.0f);

    clr += diff_intensity * SceneLights[i].kd * texture(DIFFUSE_MAP, fs_in.texcoord);

    const vec3 refvec = reflect(SceneLights[i].direction, surface_normal);
    const vec3 eyevec = normalize(-fs_in.pos_view);
    const float spec_intensity = pow(
      max(dot(refvec, eyevec), 0.0f), SpecularIntensity);

    clr += spec_intensity * SceneLights[i].ks * texture(DIFFUSE_MAP, fs_in.texcoord);
  }

  FinalFragColor = clr;
}