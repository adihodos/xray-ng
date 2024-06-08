#pragma once

#include <cstdint>

struct ImDrawData;

namespace xray {
namespace ui {

struct UserInterfaceRenderContext
{
    const ImDrawData* draw_data;
    int32_t fb_width;
    int32_t fb_height;
};

}
}
