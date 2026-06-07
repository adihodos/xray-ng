#version 460 core

#include "core/bindless.core.glsl"

layout (location = 0) in vec2 pos;
layout (location = 1) in vec2 uv;
layout (location = 2) in vec4 color;

layout (location = 0) out gl_PerVertex {
    vec4 gl_Position;
};

layout (location = 1) out VS_OUT_FS_IN {
    vec4 color;
    vec2 uv;
    flat uint textureid;
} vs_out;

void main() {
    const uint frame_idx = (g_GlobalPushConst.data) & 0xFF;
    const FrameGlobalData_t fgd = g_FrameGlobal[frame_idx].data[0];
    gl_Position = vec4(pos * fgd.ui.scale + fgd.ui.translate, 0.0f, 1.0f);
    vs_out.color = color;
    vs_out.uv = uv;
    vs_out.textureid = fgd.ui.textureid;
}
