#pragma once

#include <cstdint>
#include <span>

#include "xray/math/scalar2.hpp"

namespace xray::base {
struct MemoryArena;
}

namespace xray::ui {

struct UserInterfaceBackendCreateInfo
{
    void (*upload_callback)(const uint32_t atlasid, void* context);
    void* upload_cb_context{};
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

struct UIRenderUniform
{
    math::vec2f scale;
    math::vec2f translate;
    uint32_t textureid;
};

} // namespace xray::rendering
