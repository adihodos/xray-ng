#pragma once

#include "xray/rendering/opengl/gl_handles.hpp"
#include "xray/rendering/opengl/gpu_program.hpp"
#include "xray/rendering/rendering_fwd.hpp"
#include "xray/xray.hpp"
#include <cstdint>

namespace app {

class soubroutines_demo
{
  public:
    soubroutines_demo();

    ~soubroutines_demo();

    void draw(const xray::rendering::draw_context_t& draw_ctx);

    void update(const float delta_ms);

    void key_event(const int32_t key_code, const int32_t action, const int32_t mods) noexcept;

    explicit operator bool() const noexcept { return vertex_buffer_ && index_buffer_ && vertex_layout_ && draw_prog_; }

  private:
    void init();

  private:
    xray::rendering::scoped_buffer vertex_buffer_;
    uint32_t vertex_count_{ 0 };
    xray::rendering::scoped_buffer index_buffer_;
    uint32_t index_count_{ 0 };
    xray::rendering::scoped_vertex_array vertex_layout_;
    xray::rendering::gpu_program draw_prog_;
    bool use_ads_lighting_{ true };
    uint32_t mtl_idx_{ 0 };

  private:
    XRAY_NO_COPY(soubroutines_demo);
};

} // namespace app
