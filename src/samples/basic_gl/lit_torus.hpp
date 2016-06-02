#pragma once

#include "xray/xray.hpp"
#include "xray/rendering/opengl/gl_handles.hpp"
#include "xray/rendering/opengl/gpu_program.hpp"
#include "xray/rendering/rendering_fwd.hpp"
#include <cstdint>

namespace app {

class lit_object {
public:
  lit_object();

  ~lit_object();

  bool valid() const noexcept { return valid_; }

  explicit operator bool() const noexcept { return valid(); }

  void draw(const xray::rendering::draw_context_t& draw_ctx);

  void update(const float delta);

  void key_event(const int32_t key_code, const int32_t action,
                 const int32_t mods) noexcept;

private:
  void init();

private:
  xray::rendering::scoped_buffer vbuff_;
  xray::rendering::scoped_buffer ibuff_;
  xray::rendering::scoped_vertex_array    layout_;
  xray::rendering::gpu_program          draw_prog_;
  uint32_t                              index_count_{0};
  bool                                  valid_{false};
  float                                 rx_{0.0f};
  float                                 ry_{0.0f};

private:
  XRAY_NO_COPY(lit_object);
};

} // namespace app
