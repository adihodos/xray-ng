#pragma once

#include "xray/xray.hpp"
#include "xray/base/unique_pointer.hpp"
#include "xray/rendering/opengl/gl_handles.hpp"
#include "xray/rendering/opengl/gpu_program.hpp"
#include "xray/ui/window_context.hpp"

namespace app {

class colored_circle {
public:
  colored_circle();

  void update(const float delta) noexcept;

  void draw(const xray::ui::window_context&) noexcept;

  bool valid() const noexcept { return valid_; }

  explicit operator bool() const noexcept { return valid(); }

private:
  void init() noexcept;

private:
  xray::rendering::scoped_buffer vertex_buff_;

  xray::rendering::scoped_buffer index_buff_;

  xray::rendering::scoped_vertex_array layout_desc_;

  xray::rendering::gpu_program drawing_program_;

  bool valid_{false};

private:
  XRAY_NO_COPY(colored_circle);
};

} // namespace app
