#pragma once

#include "xray/xray.hpp"
#include "demo_base.hpp"
#include "xray/base/unique_pointer.hpp"
#include "xray/rendering/opengl/gl_handles.hpp"
#include "xray/rendering/opengl/gpu_program.hpp"
#include "xray/rendering/opengl/program_pipeline.hpp"
#include "xray/rendering/render_stage.hpp"

namespace app {

class colored_circle_demo : public demo_base {
public:
  colored_circle_demo();

  virtual void draw(const xray::rendering::draw_context_t&) override;
  virtual void update(const float delta_ms) override;
  virtual void event_handler(const xray::ui::window_event& evt) override;
  virtual void compose_ui() override;

private:
  void init() noexcept;

private:
  xray::rendering::scoped_buffer       vertex_buff_;
  xray::rendering::scoped_buffer       index_buff_;
  xray::rendering::scoped_vertex_array layout_desc_;
  xray::rendering::vertex_program      _vs;
  xray::rendering::fragment_program    _fs;
  xray::rendering::program_pipeline    _pipeline;

private:
  XRAY_NO_COPY(colored_circle_demo);
};

} // namespace app
