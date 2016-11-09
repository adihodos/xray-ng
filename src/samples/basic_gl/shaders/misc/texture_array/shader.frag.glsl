#version 450 core

layout (binding = 0) uniform sampler2DArray TEXTURE_IMAGES;
uniform int TEX_IMAGE_IDX;

in VS_OUT_PS_IN {
  layout (location = 0) vec2 texcoord;
} ps_in;

layout (location = 0) out vec4 final_frag_color;

void main() {
  final_frag_color = texture(TEXTURE_IMAGES, vec3(ps_in.texcoord, float(TEX_IMAGE_IDX))); 
    //texelFetch(TEXTURE_IMAGES, ivec3(gl_FragCoord.xy, TEX_IMAGE_IDX), 0);
} 