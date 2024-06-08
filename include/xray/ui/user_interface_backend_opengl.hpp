#pragma once

#include "xray/base/unique_pointer.hpp"
#include "xray/xray.hpp"

#include "xray/rendering/opengl/gl_handles.hpp"
#include "xray/rendering/opengl/gpu_program.hpp"
#include "xray/rendering/opengl/program_pipeline.hpp"
#include <opengl/opengl.hpp>

#include <span>
#include <tl/optional.hpp>

namespace xray {
namespace ui {

struct UserInterfaceRenderContext;

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

class UserInterfaceBackendOpengGL
{
  private:
    struct PrivateConstructionToken
    {
        explicit PrivateConstructionToken() = default;
    };

  public:
    UserInterfaceBackendOpengGL(PrivateConstructionToken,
                                xray::rendering::scoped_buffer vertex_buffer,
                                xray::rendering::scoped_buffer index_buffer,
                                xray::rendering::scoped_vertex_array vertex_arr,
                                xray::rendering::vertex_program vs,
                                xray::rendering::fragment_program fs,
                                xray::rendering::program_pipeline pipeline,
                                xray::rendering::scoped_texture font_texture,
                                xray::rendering::scoped_sampler font_sampler,
                                uint32_t vertex_buffer_size,
                                uint32_t index_buffer_size);

    UserInterfaceBackendOpengGL(const UserInterfaceBackendOpengGL&) = delete;
    UserInterfaceBackendOpengGL& operator=(const UserInterfaceBackendOpengGL&) = delete;

    UserInterfaceBackendOpengGL(UserInterfaceBackendOpengGL&&) = default;
    UserInterfaceBackendOpengGL& operator=(UserInterfaceBackendOpengGL&&) = default;

    static tl::optional<UserInterfaceBackendOpengGL> create(const UserInterfaceBackendCreateInfo&);

    GLuint font_atlas_texture_handle() const noexcept { return xray::base::raw_handle(_font_texture); }
    void render(const UserInterfaceRenderContext& render_ctx);

  private:
    xray::rendering::scoped_buffer _vertex_buffer;
    xray::rendering::scoped_buffer _index_buffer;
    xray::rendering::scoped_vertex_array _vertex_arr;
    xray::rendering::vertex_program _vs;
    xray::rendering::fragment_program _fs;
    xray::rendering::program_pipeline _pipeline;
    xray::rendering::scoped_texture _font_texture;
    xray::rendering::scoped_sampler _font_sampler;
    uint32_t _vertex_buffer_size;
    uint32_t _index_buffer_size;
};

} // namespace ui
} // namespace xray
