#version 450 core

layout (triangles) in;
layout (line_strip, max_vertices = 6) out;

layout (row_major, binding = 0) uniform TransformMatrices {
  mat4 WORLD_VIEW_PROJ;
  vec4 START_COLOR;
  vec4 END_COLOR;
  float NLENGTH;
};

in VS_OUT_GS_IN {
  vec3 pos;
  vec3 normal;
} gs_in[];

out GS_OUT_FS_IN {
  vec4 color;
} gs_out;

out gl_PerVertex {
  vec4 gl_Position;
};

void main() {
  for (uint i = 0; i < gs_in.length(); ++i) {
    gl_Position = WORLD_VIEW_PROJ * vec4(gs_in[i].pos, 1.0f);
    gs_out.color = START_COLOR;
    EmitVertex();

    gl_Position = WORLD_VIEW_PROJ 
      * vec4(gs_in[i].pos + NLENGTH * gs_in[i].normal, 1.0f);
    gs_out.color = END_COLOR;
    EmitVertex();

    EndPrimitive();
  }
}
