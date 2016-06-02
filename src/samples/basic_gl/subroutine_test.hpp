#pragma once

#include "xray/xray.hpp"
#include "xray/rendering/draw_context.hpp"
#include "xray/rendering/opengl/gl_handles.hpp"
#include "xray/rendering/opengl/gpu_program.hpp"
#include "xray/rendering/rendering_fwd.hpp"
#include <cstdint>

namespace app {

enum class shading_model : uint8_t { diffuse, phong };

class basic_object {
public:
  basic_object();

  ~basic_object();

  bool valid() const noexcept { return valid_; }

  explicit operator bool() const noexcept { return valid(); }

  void draw(const xray::rendering::draw_context_t& draw_ctx);

  void update(const float delta);

private:
  void init();

private:
  xray::rendering::scoped_buffer vertex_buffer_;
  xray::rendering::scoped_buffer index_buffer_;
  xray::rendering::scoped_vertex_array    layout_;
  xray::rendering::gpu_program          draw_prog_;
  uint32_t                              index_count_{};
  bool                                  initialized_{false};
  bool                                  valid_{false};
  shading_model                         shade_model_{shading_model::diffuse};

private:
  XRAY_NO_COPY(basic_object);
};

} // namespace app