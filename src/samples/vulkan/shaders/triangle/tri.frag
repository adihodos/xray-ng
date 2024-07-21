#version 460 core

in VS_OUT_FS_IN {
    layout (location = 0) vec3 color;
} fs_in;

layout (set = 0, binding = 0, rgba32f) uniform textureBuffer TexelBuffer;
layout (set = 0, binding = 1) uniform sampler TexelBufferSampler;

layout (push_constant, row_major) uniform ShaderGlobalData {
    mat2 rotation;
    uint w;
    uint h;
} g_frag_data;

layout (location = 0) out vec4 FinalFragColor;

void main() {
    const vec2 xy = ivec2(gl_FragCoord.xy);
    vec4 color = imageLoad(TexelBuffer, xy.y * g_frag_data.w + xy.x);
    //FinalFragColor = vec4(g_frag_data.color, 1.0);
    FinalFragColor = color;
}
