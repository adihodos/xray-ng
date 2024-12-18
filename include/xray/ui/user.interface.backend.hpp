#pragma once

#include <cstdint>
#include <span>

namespace xray::ui {

struct UserInterfaceBackendCreateInfo
{
    std::span<const uint8_t> font_atlas_pixels;
    uint32_t atlas_width;
    uint32_t atlas_height;
    uint32_t max_vertices;
    uint32_t max_indices;
    uint32_t vertex_size;
    uint8_t offset0;
    uint8_t offset1;
    uint8_t offset2;
};

} // namespace xray::rendering
