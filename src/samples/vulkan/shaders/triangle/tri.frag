#version 460 core

#extension GL_EXT_nonuniform_qualifier : require

struct FrameGlobalData_t {
    mat4 world_view_proj;
    mat4 projection;
    mat4 inv_projection;
    mat4 view;
    mat4 ortho_proj;
    vec3 eye_pos;
    uint frame_id;
};

layout (std140, set = 0, binding  = 0) uniform FrameGlobal {
    FrameGlobalData_t data[];
} g_FrameGlobal[];

struct InstanceRenderInfo_t {
    mat4 model;
    uint vtx_buff;
    uint idx_buff;
    uint mtl_coll_offset;
    uint mtl_id;
};

layout (set = 1, binding = 0) readonly buffer InstancesGlobal {
    InstanceRenderInfo_t data[];
} g_InstanceGlobal[];

struct VertexPTC {
    vec2 pos;
    vec2 uv;
    vec4 color;
};

layout (set = 1, binding = 0) readonly buffer VerticesGlobal {
    VertexPTC data[];
} g_VertexBufferGlobal[];

layout (set = 1, binding = 0) readonly buffer IndicesGlobal {
    uint data[];
} g_IndexBufferGlobal[];

layout (set = 2, binding = 0) uniform sampler2D g_Textures2DGlobal[];
layout (set = 2, binding = 0) uniform sampler2DArray g_Textures2DArrayGlobal[];
layout (set = 2, binding = 0) uniform samplerCube g_TexturesCubeGlobal[];

layout (push_constant) uniform PushConstantsGlobal {
    uint data;
} g_GlobalPushConst;

in VS_OUT_FS_IN {
    layout (location = 0) vec2 uv;
    layout (location = 1) flat uint mtl;
} fs_in;

layout (location = 0) out vec4 FinalFragColor;

void main() {
    FinalFragColor = texture(g_Textures2DGlobal[fs_in.mtl], fs_in.uv);
}
