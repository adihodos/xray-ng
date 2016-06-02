#pragma once

#include "xray/xray.hpp"
#include "xray/rendering/opengl/gl_handles.hpp"
#include "xray/rendering/opengl/gpu_program.hpp"
#include "xray/ui/window_context.hpp"
#include <cstdint>

namespace app {

struct fractal_params {
  float    width{0.0f};
  float    height{0.0f};
  float    shape_re{0.0f};
  float    shape_im{0.0f};
  uint32_t iterations{0};
};

class fractal_painter {
public:
  fractal_painter();

  ~fractal_painter();

  /// \name Events
  /// @{

public:
  void draw(const xray::ui::window_context& draw_ctx);

  void key_press(const int32_t key_code);
  /// @}

  bool valid() const noexcept { return initialized_; }

  explicit operator bool() const noexcept { return valid(); }

private:
  void init();

private:
  xray::rendering::scoped_buffer quad_vb_;
  xray::rendering::scoped_buffer quad_ib_;
  xray::rendering::scoped_vertex_array    quad_layout_;
  xray::rendering::gpu_program          quad_draw_prg_;
  bool                                  initialized_{false};
  uint32_t                              shape_idx_{0u};
  uint32_t                              iter_sel_{5u};
  fractal_params                        fp_{};

private:
  XRAY_NO_COPY(fractal_painter);
};

} // namespace app