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
    layout (location = 1) flat uint mtl;
} vs_out;

void main() {
    const uint frame_idx = (g_GlobalPushConst.data) & 0xFF;
    const FrameGlobalData_t fgd = g_FrameGlobal[frame_idx].data[0];

    const InstanceRenderInfo_t inst = g_InstanceGlobal[frame_idx].data[gl_InstanceIndex];
    // const uint vertex_index = g_IndexBufferGlobal[inst.idx_buff].data[gl_VertexIndex];
    // const VertexPBR vtx = g_VertexBufferPBRGlobal[inst.vtx_buff].data[vertex_index];

    // gl_Position = fgd.world_view_proj * inst.model * vec4(vtx.pos, 1.0);
    // vs_out.uv = vtx.uv;

    gl_Position = fgd.world_view_proj * inst.model * vec4(pos, 1.0);
    vs_out.uv = uv;
    // vs_out.mtl = pbr;
    vs_out.mtl = inst.mtl_id;
}

