//
// lights data
const uint dir_lights_sbo = g_FrameGlobal[fs_in.fii.x].data[0].lights.sbo_directional_lights;
const uint dir_lights_cnt = g_FrameGlobal[fs_in.fii.x].data[0].lights.directional_lights_count;
const uint point_lights_sbo = g_FrameGlobal[fs_in.fii.x].data[0].lights.sbo_point_lights;
const uint point_lights_cnt = g_FrameGlobal[fs_in.fii.x].data[0].lights.point_lights_count;

const uvec2 dls = uvec2(dir_lights_cnt, dir_lights_sbo);
const uvec2 pls = uvec2(point_lights_cnt, point_lights_sbo);

