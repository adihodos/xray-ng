#version 460 core

#include "core/bindless.core.glsl"
#include "defs.glsl"

layout (location = 0) in vec3 pos;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec4 color;
layout (location = 3) in vec4 tangent;
layout (location = 4) in vec2 uv;
layout (location = 5) in uint pbr;

out gl_PerVertex {
    vec4 gl_Position;
};

out VS_OUT_FS_IN {
    layout (location = 0) vec2 uv;
    layout (location = 1) flat uint mtl_buffer_elem;
    layout (location = 2) flat uint mtl_buffer;
} vs_out;

void main() {

    const uint frame_idx = (g_GlobalPushConst.data) & 0xFF;
    const uint inst_buffer_idx = (g_GlobalPushConst.data & 0xFFFF0000) >> 16;
    const uint instance_index = (g_GlobalPushConst.data & 0x0000FF00) >> 8;

    const FrameGlobalData_t fgd = g_FrameGlobal[frame_idx].data[0];
    const InstanceRenderInfo_t inst = g_InstanceGlobal[inst_buffer_idx].data[instance_index];
    // const uint vertex_index = g_IndexBufferGlobal[inst.idx_buff].data[gl_VertexIndex];
    // const VertexPBR vtx = g_VertexBufferPBRGlobal[inst.vtx_buff].data[vertex_index];

    // gl_Position = fgd.world_view_proj * inst.model * vec4(vtx.pos, 1.0);
    // vs_out.uv = vtx.uv;

    gl_Position = fgd.world_view_proj * inst.model * vec4(pos, 1.0);
    vs_out.uv = uv;
    vs_out.mtl_buffer_elem = pbr;
    vs_out.mtl_buffer = inst.mtl_buffer;
    // vs_out.mtl = inst.mtl_id;
}

