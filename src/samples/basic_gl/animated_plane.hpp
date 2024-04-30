#pragma once

#include "xray/rendering/opengl/gl_handles.hpp"
#include "xray/rendering/opengl/gpu_program.hpp"
#include "xray/rendering/rendering_fwd.hpp"
#include "xray/xray.hpp"
#include "xray/xray_types.hpp"

namespace app {

class animated_paper_plane
{
  public:
    animated_paper_plane() noexcept { init(); }

    void update(const xray::scalar_lowp delta) noexcept;

    void draw(const xray::rendering::draw_context_t&) noexcept;

    bool valid() const noexcept { return valid_; }

    explicit operator bool() const noexcept { return valid(); }

  private:
    void init() noexcept;

  private:
    xray::rendering::scoped_buffer vertex_buff_;

    xray::rendering::scoped_buffer index_buff_;

    xray::rendering::scoped_vertex_array layout_desc_;

    xray::rendering::gpu_program drawing_program_;

    float rotation_angle_{ 0.0f };

    bool valid_{ false };

  private:
    XRAY_NO_COPY(animated_paper_plane);
};

} // namespace app
