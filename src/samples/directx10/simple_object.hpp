#pragma once

#include "xray/rendering/rendering_fwd.hpp"
#include "xray/xray.hpp"

#include "fragment_shader.hpp"
#include "gpu_buffer.hpp"
#include "rasterizer_state.hpp"
#include "renderer_dx10.hpp"
#include "vertex_layout_decl.hpp"
#include "vertex_shader.hpp"

namespace xray {
namespace rendering {
namespace direct10 {
class renderer;
}
}
}

namespace app {

class simple_object
{
  public:
    explicit simple_object(xray::rendering::directx10::renderer* r_ptr);

    ~simple_object();

    bool valid() const noexcept { return valid_; }

    explicit operator bool() const noexcept { return valid(); }

    void update(const float delta) noexcept;

    void draw(const xray::rendering::draw_context_t&);

  private:
    void init(xray::rendering::renderer* r_ptr);

  private:
    xray::rendering::gpu_buffer vertex_buff_{};
    xray::rendering::gpu_buffer index_buff_{};
    xray::rendering::gpu_buffer cbuffer_{};
    xray::rendering::vertex_shader vert_shader_;
    xray::rendering::fragment_shader frag_shader_;
    xray::rendering::scoped_vertex_layout_decl layout_decl_;
    xray::rendering::directx10::renderer* r_ptr_;
    xray::rendering::scoped_rasterizer_state rstate_;
    bool valid_{ false };
    float angle_{ 0.0f };

  private:
    XRAY_NO_COPY(simple_object);
};

} // namespace app