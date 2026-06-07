
//
// no include guard
// unpack frame data
const uint frame_idx = (g_GlobalPushConst.data) & 0xFF;
const uint inst_buffer_idx = (g_GlobalPushConst.data & 0xFFFF0000) >> 16;
const uint instance_index = (g_GlobalPushConst.data & 0x0000FF00) >> 8;
const FrameGlobalData_t fgd = g_FrameGlobal[frame_idx].data[0];
const InstanceRenderInfo_t inst = g_InstanceGlobal[inst_buffer_idx].data[instance_index];

