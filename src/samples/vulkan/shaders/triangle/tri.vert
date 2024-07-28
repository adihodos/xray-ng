#version 460 core

struct VertexPT {
    vec2 pos;
    vec2 uv;
};

const VertexPT kTriangleVertices[] = {
    VertexPT ( vec2(-1, -1), vec2(0, 0) ),
    VertexPT ( vec2(1, -1), vec2(1, 0) ),
    VertexPT ( vec2(0, 1), vec2(0.5, 1) )
};

// const vec2 TriangleVertices[] = vec2[](
//     vec2(-1.0, -1.0),
//     vec2(1.0, -1.0),
//     vec2(0.0, 1.0)
// );
//
// const vec3 TriangleColors[] = vec3[](
//     vec3(1.0, 0.0, 0.0),
//     vec3(0.0, 1.0, 0.0),
//     vec3(0.0, 0.0, 1.0)
// );

out gl_PerVertex {
    vec4 gl_Position;
};

out VS_OUT_FS_IN {
    layout (location = 0) vec2 uv;
} vs_out;

layout (push_constant, row_major) uniform ShaderGlobalData {
    mat2 rotation;
    uint w;
    uint h;
} g_data;

layout (set = 0, binding = 0) uniform textureBuffer TexelBuffer;

void main() {
    const vec2 pos = g_data.rotation * kTriangleVertices[gl_VertexIndex].pos;
    gl_Position = vec4(pos.xy, 0.0, 1.0);
    vs_out.uv = g_data.rotation * kTriangleVertices[gl_VertexIndex].uv;
    // vs_out.color = TriangleColors[gl_VertexIndex];
}

