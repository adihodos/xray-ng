#version 450 core

layout (location = 0) in vec3 vs_in_pos;
layout (location = 1) in vec3 vs_in_normal;
layout (location = 2) in vec2 vs_in_texc;

layout (row_major, binding = 0) uniform TransformMatrices {
  mat4 MVPMatrix;
  mat4 ModelMatrix;
  mat4 NormalMatrix;
  vec3 EyePosition;
};

out gl_PerVertex {
  vec4 gl_Position;
};

out VS_OUT_FS_IN {
	vec3 N; // normal
	vec3 V; // view vector (from point to eye)
	vec3 P; // point position
	vec2 TexCoord;
} vs_out;

void main() {
  gl_Position = MVPMatrix * vec4(vs_in_pos, 1.0f);
  // normal in world space
  vs_out.N = normalize((NormalMatrix * vec4(vs_in_normal, 0.0f)).xyz);
  // vertex pos in world space
  vs_out.P = (ModelMatrix * vec4(vs_in_pos, 1.0f)).xyz;
  // view vector from point to eye in model space
  vs_out.V = normalize(EyePosition - vs_out.P);
  vs_out.TexCoord = vs_in_texc;
  
}
