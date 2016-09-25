#version 450 core

in VS_OUT {
  layout (location = 0) vec3 position;
#ifdef VERTEX_FORMAT_HAS_NORMALS
  layout (location = 1) vec3 normal;
#endif

#ifdef VERTEX_FORMAT_HAS_TEXCOORDS
  layout (location = 2) vec2 texcoords;
#endif  
} fs_in;

layout (location = 0) out vec4 frag_color;

subroutine vec3 ColoringStyle(in const vec2 frag_coord);

subroutine uniform ColoringStyle kDrawingStyle;
uniform sampler2D kSourceTexture;
uniform float kEdgeTresholdSquared;
uniform vec2 kSurfaceSize;

float luminance(in const vec3 pixel) {
  return dot(pixel, vec3(0.2126, 0.7152, 0.0722));
}

subroutine(ColoringStyle)
vec3 color_default(in const vec2 frag_coord) {
  const vec2 texcoords = vec2(
    frag_coord.x / kSurfaceSize.x, frag_coord.y / kSurfaceSize.y);

  return texture(kSourceTexture, texcoords).rgb;
}

subroutine(ColoringStyle)
vec3 color_edges_sobel(in const vec2 frag_coord) {
  const ivec2 pixcoord = ivec2(frag_coord);
  const float s00 = luminance(
    texelFetchOffset(kSourceTexture, pixcoord, 0, ivec2(-1, 1)).rgb);
  const float s02 = luminance(
    texelFetchOffset(kSourceTexture, pixcoord, 0, ivec2(1, 1)).rgb);
  const float s10 = luminance(
    texelFetchOffset(kSourceTexture, pixcoord, 0, ivec2(-1, 0)).rgb);
  const float s12 = luminance(
    texelFetchOffset(kSourceTexture, pixcoord, 0, ivec2(1, 0)).rgb);
  const float s20 = luminance( 
    texelFetchOffset(kSourceTexture, pixcoord, 0, ivec2(-1, -1)).rgb);
  const float s22 = luminance(
    texelFetchOffset(kSourceTexture, pixcoord, 0, ivec2(1, -1)).rgb);

  const float s01 = luminance(
    texelFetchOffset(kSourceTexture, pixcoord, 0, ivec2(0, 1)).rgb);
  const float s21 = luminance(
    texelFetchOffset(kSourceTexture, pixcoord, 0, ivec2(0, -1)).rgb);

  const float sx = -1.0 * s00 + s02 - 2.0 * s10 + 2.0 * s12 - 1.0 * s20 + s22;
  const float sy = -1.0 * s00 - 2.0 * s01 - 1.0 * s02 + s20 + 2.0 * s21 + s22;
  const float threshold = sx * sx + sy * sy;

  const float val = 1.0 * float(threshold > kEdgeTresholdSquared);
  return vec3(val);
}

const float kMaskFactor[] = {
  0.35355339, 0.35355339, 0.35355339, 0.35355339, 0.5, 0.5, 0.1666,
  0.1666, 0.33333
};

// sqrt(2) = 1.414213
const mat3 kMasks[] = mat3[](
  mat3(
    1.0, 1.414213, 1, 
    0, 0, 0,
    -1, -1.414213, -1)
  ,
  mat3(
    1, 0, -1,
    1.414213, 0, -1.414213,
    1, 0, -1
  )
  ,
  mat3(
    0, -1, 1.414213,
    1, 0, -1,
    1.414213, 1, 0
  )
  ,
  mat3(
    1.414213, -1, 0,
    -1, 0, 1,
    0, 1, -1.414213
  )
  ,
  mat3(
    0, 1, 0,
    -1, 0, -1,
    0, 1, 0
  )
  ,
  mat3(
    -1, 0, 1,
    0, 0, 0,
    1, 0, -1
  )
  ,
  mat3(
    1, -2, 1,
    -2, 4, -2,
    1, -2, 1
  )
  ,
  mat3(
    -2, 1, -2,
    1, 4, 1,
    -2, 1, -2
  )
  ,
  mat3(
    1, 1, 1,
    1, 1, 1,
    1, 1, 1
    )
);

subroutine(ColoringStyle)
vec3 color_edges_frei_chen(in const vec2 frag_coord) {
  const ivec2 px = ivec2(frag_coord);

  const mat3 texels = mat3(
    luminance(texelFetchOffset(kSourceTexture, px, 0, ivec2(-1, 1)).rgb),
    luminance(texelFetchOffset(kSourceTexture, px, 0, ivec2(0, 1)).rgb),
    luminance(texelFetchOffset(kSourceTexture, px, 0, ivec2(1, 1)).rgb),

    luminance(texelFetchOffset(kSourceTexture, px, 0, ivec2(-1, 0)).rgb),
    luminance(texelFetchOffset(kSourceTexture, px, 0, ivec2(0, 0)).rgb),
    luminance(texelFetchOffset(kSourceTexture, px, 0, ivec2(1, 0)).rgb),

    luminance(texelFetchOffset(kSourceTexture, px, 0, ivec2(-1, -1)).rgb),
    luminance(texelFetchOffset(kSourceTexture, px, 0, ivec2(0, -1)).rgb),
    luminance(texelFetchOffset(kSourceTexture, px, 0, ivec2(1, -1)).rgb)
  );

  float conv[9];
  for (int i = 0; i < 9; ++i) {
    const mat3 mtx = kMasks[i] * kMaskFactor[i];
    const float dp = dot(mtx[0], texels[0]) + dot(mtx[1], texels[1])
      + dot(mtx[2], texels[2]);
    conv[i] = dp * dp;
  }

  const float M = conv[0] + conv[1] + conv[2] + conv[3];
  const float S = M + conv[4] + conv[5] + conv[6] + conv[7] + conv[8]; 

  return vec3(sqrt(M / S));
}

void main() {
  frag_color = vec4(kDrawingStyle(gl_FragCoord.xy), 1.0);
}