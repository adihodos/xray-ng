#version 460 core

const vec2 TriangleVertices[] = vec2[](
    vec2(-1.0, -1.0),
    vec2(1.0, -1.0),
    vec2(0.0, 1.0)
);

const vec3 TriangleColors[] = vec3[](
    vec3(1.0, 0.0, 0.0),
    vec3(0.0, 1.0, 0.0),
    vec3(0.0, 0.0, 1.0)
);

out gl_PerVertex {
    vec4 gl_Position;
};

out VS_OUT_FS_IN {
    layout (location = 0) vec3 color;
} vs_out;

layout (push_constant, row_major) uniform ShaderGlobalData {
    mat2 rotation;
    uint w;
    uint h;
} g_data;

layout (set = 0, binding = 0) uniform textureBuffer TexelBuffer;

void main() {
    const vec2 pos = g_data.rotation * TriangleVertices[gl_VertexIndex];
    gl_Position = vec4(pos.xy, 0.0, 1.0);
    // vs_out.color = TriangleColors[gl_VertexIndex];
}

