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
    alignas(16) xray::math::mat4f world_view_proj;
    alignas(16) xray::math::mat4f projection;
    alignas(16) xray::math::mat4f inv_projection;
    alignas(16) xray::math::mat4f view;
    alignas(16) xray::math::mat4f ortho;
    alignas(16) xray::math::vec3f eye_pos;
    uint32_t frame;
    uint32_t global_color_texture;
    alignas(16) UIData ui;
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
