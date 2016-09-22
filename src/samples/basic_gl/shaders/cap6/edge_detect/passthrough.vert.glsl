#version 450 core

layout (location = 0) in vec3 vs_in_pos;

#ifdef VERTEX_FORMAT_HAS_NORMALS
layout (location = 1) in vec3 vs_in_normal;
#endif

#ifdef VERTEX_FORMAT_HAS_TEXCOORDS
layout (location = 2) in vec2 vs_in_texcoords;
#endif

out VS_OUT {
  layout (location = 0) vec3 position;
#ifdef VERTEX_FORMAT_HAS_NORMALS
  layout (location = 1) vec3 normal;
#endif

#ifdef VERTEX_FORMAT_HAS_TEXCOORDS
  layout (location = 2) vec2 texcoords;
#endif    
} vs_out;

out gl_PerVertex {
  vec4 gl_Position;
};

void main() {
  gl_Position = vec4(vs_in_pos, 1.0);
  vs_out.position = vs_in_pos;

#ifdef VERTEX_FORMAT_HAS_NORMALS
  vs_out.normal = vs_in_normal;
#endif

#ifdef VERTEX_FORMAT_HAS_TEXCOORDS
  vs_out.texcoords = vs_in_texcoords;
#endif    
}