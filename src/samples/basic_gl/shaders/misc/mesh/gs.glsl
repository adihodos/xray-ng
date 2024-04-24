#version 450 core

layout (triangles) in;
layout (line_strip, max_vertices = 6) out;

in VS_OUT_GS_IN {
  vec3 pos;
  vec3 normal;
} gs_in[3];

out gl_PerVertex {
  vec4 gl_Position;
};

out GS_OUT_PS_IN {
  vec4 color;
} gs_out;

layout (std140, row_major, binding = 0) uniform TransformMatrices {
  mat4 WORLD_VIEW_PROJECTION;
  vec4 COLOR_START;
  vec4 COLOR_END;
  float NORMAL_LENGTH;
};

void main() {

  for (int i = 0; i < 3; ++i) {
    gl_Position = WORLD_VIEW_PROJECTION * vec4(gs_in[i].pos, 1.0f);
    gs_out.color = COLOR_START;

    EmitVertex();

    vec3 end_pos = gs_in[i].pos + NORMAL_LENGTH * gs_in[i].normal;
    gl_Position = WORLD_VIEW_PROJECTION * vec4(end_pos, 1.0f);
    gs_out.color = COLOR_END;

    EmitVertex();

    EndPrimitive();
  }
}
