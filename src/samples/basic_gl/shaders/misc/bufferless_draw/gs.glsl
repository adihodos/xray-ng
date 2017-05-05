#version 450 core

layout (points) in;
layout (triangle_strip, max_vertices = 3) out;

in VS_OUT_GS_IN {
  int vertexid;
} gs_in[];

out gl_PerVertex {
  vec4 gl_Position;
};

out GS_OUT_FS_IN {
  vec4 color;
} gs_out;

void main() {
  const vec2 QUAD_VERTS[6] = vec2[6](
    vec2(-1.0f, -1.0f), vec2(+1.0f, +1.0f), vec2(-1.0f, +1.0f),
    vec2(-1.0f, -1.0f), vec2(+1.0f, -1.0f), vec2(+1.0f, +1.0f)
  );

  const vec4 COLORS[4] = vec4[4](
    vec4(1.0f, 0.0f, 0.0f, 1.0f), vec4(0.0f, 1.0f, 0.0f, 1.0f),
    vec4(0.0f, 0.0f, 1.0f, 1.0f), vec4(1.0f, 0.0f, 0.0f, 1.0f)
  );

  const int id = gs_in[0].vertexid;

  gl_Position = vec4(QUAD_VERTS[id * 3 + 0], 0.0f, 1.0f);
  gs_out.color = COLORS[id + 0];
  EmitVertex();

  gl_Position = vec4(QUAD_VERTS[id * 3 + 1], 0.0f, 1.0f);
  gs_out.color = COLORS[id + 1];
  EmitVertex();

  gl_Position = vec4(QUAD_VERTS[id * 3 + 2], 0.0f, 1.0f);
  gs_out.color = COLORS[id + 2];
  EmitVertex();

  EndPrimitive();
}