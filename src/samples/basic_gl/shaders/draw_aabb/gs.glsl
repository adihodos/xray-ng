#version 450 core

layout (points) in;
// layout (line_strip, max_vertices = 2) out;
layout (triangle_strip, max_vertices = 6) out;

in VS_OUT_GS_IN {
  vec3 pos;
} gs_in[];

out gl_PerVertex {
  vec4 gl_Position;
};

out GS_OUT_FS_IN {
  vec4 color;
} gs_out;

layout (std140, row_major) uniform DrawParams {
  mat4 WORLD_VIEW_PROJECTION;
  vec4 LINE_STARTCOLOR;
  vec4 LINE_ENDCOLOR;
  float W;
  float H;
  float D;
};

void main() {
  const vec3 vertices[] = vec3[8](
    // vec3(-W, -H, -D),
    // vec3(W, -H, -D),
    // vec3(W, H, -D),
    // vec3(-W, H, -D),
    // vec3(W, -H, D),
    // vec3(W, H, D),
    // vec3(-W, H, D),
    // vec3(-W, -H, D)
    vec3(-1, -1, -1),
    vec3(1, -1, -1),
    vec3(1, 1, -1),
    vec3(-1, 1, -1),
    vec3(1, -1, 1),
    vec3(1, 1, 1),
    vec3(-1, 1, 1),
    vec3(-1, -1, 1)
  );

  const ivec2 indices[] = ivec2[](
    ivec2(0, 1), ivec2(1, 2), ivec2(2, 3), ivec2(3, 0),
    ivec2(1, 4), ivec2(4, 5), ivec2(5, 2), 
    ivec2(5, 6), ivec2(6, 7), ivec2(7, 4),
    ivec2(6, 3), ivec2(0, 7)
  );

  // for (int i = 0; i < indices.length(); ++i) {
  //   gl_Position = WORLD_VIEW_PROJECTION * vec4(-1.0f, -1.0f, -1.0f, 1.0f);
  //   gs_out.color = LINE_STARTCOLOR;
  //   EmitVertex();

  //   gl_Position = WORLD_VIEW_PROJECTION * vec4(1.0f, 1.0f, 1.0f, 1.0f);
  //   gs_out.color = LINE_ENDCOLOR;
  //   EmitVertex();

  //   EndPrimitive();
  // }

  // gl_Position = WORLD_VIEW_PROJECTION * vec4(-1.0f, -1.0f, -1.0f, 1.0f);
  // gs_out.color = LINE_STARTCOLOR;
  // EmitVertex();

  // gl_Position = WORLD_VIEW_PROJECTION * vec4(1.0f, 1.0f, 1.0f, 1.0f);
  // gs_out.color = LINE_ENDCOLOR;
  // EmitVertex();

  // EndPrimitive();

  gl_Position = vec4(-1.0f, -1.0f, 0.0f, 1.0f);
  gs_out.color = vec4(1.0f, 0.0f, 0.0f, 1.0f);
  EmitVertex();

  gl_Position = vec4(-1.0f, 1.0f, 0.0f, 1.0f);
  gs_out.color = vec4(0.0f, 1.0f, 0.0f, 1.0f);
  EmitVertex();

  gl_Position = vec4(1.0f, 1.0f, 0.0f, 1.0f);
  gs_out.color = vec4(0.0f, 0.0f, 1.0f, 1.0f);
  EmitVertex();
  EndPrimitive();

  gl_Position = vec4(-1.0f, -1.0f, 0.0f, 1.0f);
  gs_out.color = vec4(1.0f, 1.0f, 0.0f, 1.0f);
  EmitVertex();

  gl_Position = vec4(1.0f, 1.0f, 0.0f, 1.0f);
  gs_out.color = vec4(0.0f, 1.0f, 1.0f, 1.0f);
  EmitVertex();

  gl_Position = vec4(1.0f, -1.0f, 0.0f, 1.0f);
  gs_out.color = vec4(1.0f, 0.0f, 1.0f, 1.0f);
  EmitVertex();
  EndPrimitive();
}