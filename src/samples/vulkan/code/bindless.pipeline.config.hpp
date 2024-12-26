#pragma once

#include <cstdint>

#include "xray/math/scalar2.hpp"
#include "xray/math/scalar3.hpp"
#include "xray/math/scalar4x4.hpp"

namespace app {

struct UIData
{
    xray::math::vec2f scale;
    xray::math::vec2f translate;
    uint32_t textureid;
};

struct FrameGlobalData
{
    xray::math::mat4f world_view_proj;
    xray::math::mat4f projection;
    xray::math::mat4f inv_projection;
    xray::math::mat4f view;
    xray::math::mat4f ortho;
    xray::math::vec3f eye_pos;
    uint32_t frame;
    UIData ui;
};

struct alignas(16) InstanceRenderInfo
{
    xray::math::mat4f model;
    uint32_t vtx_buff;
    uint32_t idx_buff;
    uint32_t mtl_coll;
    uint32_t mtl;
};

}
