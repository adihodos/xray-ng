#version 460 core

in VS_OUT_FS_IN {
    layout (location = 0) vec2 uv;
} fs_in;

layout (set = 4, binding = 0) uniform sampler2D g_MiscTextures[];

layout (push_constant, row_major) uniform ShaderGlobalData {
    mat2 rotation;
    uint w;
    uint h;
} g_frag_data;

layout (location = 0) out vec4 FinalFragColor;

void main() {

    // const vec2 xy = ivec2(gl_FragCoord.xy);
    // vec4 color = imageLoad(TexelBuffer, xy.y * g_frag_data.w + xy.x);
    //FinalFragColor = vec4(g_frag_data.color, 1.0);
    FinalFragColor = texture(g_MiscTextures[0], fs_in.uv);
}
