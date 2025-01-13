#version 460 core

#include "core/bindless.core.glsl"

layout (location = 0) in vec3 pos;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec3 tangent;
layout (location = 3) in vec2 texcoords;

out gl_PerVertex {
    vec4 gl_Position;
};

out VS_OUT_FS_IN {
    layout (location = 0) vec2 uv;
    layout (location = 1) flat uint colortex;
    layout (location = 2) flat uint texel;
} vs_out;

void main() {
    const uint frame_idx = (g_GlobalPushConst.data) & 0xFF;
    const uint inst_buffer_idx = (g_GlobalPushConst.data & 0xFFFF0000) >> 16;
    const uint instance_index = (g_GlobalPushConst.data & 0x0000FF00) >> 8;
    const FrameGlobalData_t fgd = g_FrameGlobal[frame_idx].data[0];

    const InstanceRenderInfo_t inst = g_InstanceGlobal[inst_buffer_idx].data[instance_index];
    gl_Position = fgd.world_view_proj * inst.model * vec4(pos, 1.0);
    vs_out.uv = texcoords;
    vs_out.colortex = fgd.global_color_texture;
    vs_out.texel = g_MaterialADSColoredGlobal[inst.mtl_buffer].data[inst.mtl_buffer_elem].texel_offset;
}
