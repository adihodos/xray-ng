#version 460 core

#extension GL_EXT_nonuniform_qualifier : require
#extension GL_EXT_debug_printf : enable 

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

out gl_PerVertex {
    vec4 gl_Position;
};

out VS_OUT_FS_IN {
    layout (location = 0) vec2 uv;
    layout (location = 1) flat uint mtl;
} vs_out;

void main() {
    const uint frame_idx = (g_GlobalPushConst.data) & 0xFF;
    const FrameGlobalData_t fgd = g_FrameGlobal[frame_idx].data[0];

    const InstanceRenderInfo_t inst = g_InstanceGlobal[frame_idx].data[gl_InstanceIndex];
    const uint vertex_index = g_IndexBufferGlobal[inst.idx_buff].data[gl_VertexIndex];
    const VertexPTC vtx = g_VertexBufferGlobal[inst.vtx_buff].data[vertex_index];

    debugPrintfEXT("frame idx = %d, vertex index = %d", frame_idx, vertex_index);

    gl_Position = inst.model * vec4(vtx.pos, 0.0, 1.0);
    vs_out.uv = vtx.uv;
    vs_out.mtl = inst.mtl_id;
}

