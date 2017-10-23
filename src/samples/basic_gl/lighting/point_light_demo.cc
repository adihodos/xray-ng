//
// Copyright (c) 2011-2017 Adrian Hodos
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above copyright
//       notice, this list of conditions and the following disclaimer in the
//       documentation and/or other materials provided with the distribution.
//     * Neither the name of the author nor the
//       names of its contributors may be used to endorse or promote products
//       derived from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR THE CONTRIBUTORS BE LIABLE FOR
// ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

/// \file point_light_demo.cc

#include "point_light_demo.hpp"
#include "xray/xray.hpp"
#include "demo_base.hpp"
#include "xray/base/app_config.hpp"
#include "xray/math/constants.hpp"
#include "xray/math/scalar3.hpp"
#include "xray/math/scalar3_math.hpp"
#include "xray/rendering/colors/color_palettes.hpp"
#include "xray/rendering/colors/rgb_color.hpp"
#include "xray/rendering/geometry/geometry_data.hpp"
#include "xray/rendering/geometry/geometry_factory.hpp"
#include "xray/rendering/opengl/gl_handles.hpp"
#include "xray/rendering/opengl/gpu_program.hpp"
#include "xray/rendering/opengl/program_pipeline.hpp"
#include "xray/rendering/opengl/scoped_resource_mapping.hpp"
#include "xray/rendering/vertex_format/vertex_pnt.hpp"
#include <algorithm>
#include <cstdint>
#include <imgui/imgui.h>

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_IMPLEMENTATION
#include <nuklear/nuklear.h>

using namespace xray::base;
using namespace xray::rendering;
using namespace xray::math;
using namespace xray::ui;
using namespace std;

extern xray::base::app_config* xr_app_config;

class user_interface {
public:
  user_interface();

private:
  void upload_font_atlas(const void* pixels, const xray::math::vec2i32& size);

  struct render_state {
    nk_buffer                            cmds;
    nk_draw_null_texture                 null;
    nk_font*                             font;
    nk_font_atlas                        font_atlas;
    nk_context                           ctx;
    xray::rendering::scoped_buffer       vb;
    xray::rendering::scoped_buffer       ib;
    xray::rendering::scoped_vertex_array vao;
    xray::rendering::vertex_program      vs;
    xray::rendering::fragment_program    fs;
    xray::rendering::program_pipeline    pp;
    xray::rendering::scoped_texture      font_tex;
    xray::rendering::scoped_sampler      sampler;
  } _renderer;
};

user_interface::user_interface() {
  nk_buffer_init_default(&_renderer.cmds);
  nk_font_atlas_init_default(&_renderer.font_atlas);

  nk_font_atlas_begin(&_renderer.font_atlas);
  _renderer.font =
    nk_font_atlas_add_default(&_renderer.font_atlas, 14.0f, nullptr);

  vec2i32 atlas_size{};
  auto    image = nk_font_atlas_bake(
    &_renderer.font_atlas, &atlas_size.x, &atlas_size.y, NK_FONT_ATLAS_RGBA32);

  upload_font_atlas(image, atlas_size);
  nk_font_atlas_end(
    &_renderer.font_atlas,
    nk_handle_id(static_cast<int32_t>(raw_handle(_renderer.font_tex))),
    &_renderer.null);

  nk_init_default(&_renderer.ctx, &_renderer.font->handle);
}

void user_interface::upload_font_atlas(const void*                pixels,
                                       const xray::math::vec2i32& size) {
  gl::CreateTextures(gl::TEXTURE_2D, 1, raw_handle_ptr(_renderer.font_tex));
  gl::TextureStorage2D(
    raw_handle(_renderer.font_tex), 1, gl::RGBA8, size.x, size.y);
  //  gl::TextureSubImage2D(raw_handle(_renderer.font_tex), )
}

app::point_light_demo::point_light_demo() { init(); }

app::point_light_demo::~point_light_demo() {}

void app::point_light_demo::init() {
  assert(!valid());

  static constexpr auto MODEL_FILE =
    //"SuperCobra.3ds";
    //"f4/f4phantom.obj";
    //"sa23/sa23_aurora.obj";
    //"cube_rounded.obj";
    //"stanford/dragon/dragon.obj";
    "stanford/teapot/teapot.obj";
  //"stanford/sportscar/sportsCar.obj";
  //"stanford/sibenik/sibenik.obj";
  //"stanford/rungholt/rungholt.obj";
  //"stanford/lost-empire/lost_empire.obj";
  //"stanford/cube/cube.obj";
  //"stanford/head/head.OBJ";

  //_meshes[obj_type::teapot] =
  //  basic_mesh{xr_app_config->model_path(MODEL_FILE).c_str()};

  geometry_data_t gblobs[2];

  geometry_factory::torus(16.0f, 8.0f, 32u, 64u, &gblobs[0]);
  geometry_factory::grid(16.0f, 16.0f, 128, 128, &gblobs[1]);

  const vertex_ripple_parameters rparams{
    1.0f, 3.0f * two_pi<float>, 16.0f, 16.0f};

  vertex_effect::ripple(
    gsl::span<vertex_pntt>{raw_ptr(gblobs[1].geometry),
                           raw_ptr(gblobs[1].geometry) +
                             gblobs[1].vertex_count},
    gsl::span<uint32_t>{raw_ptr(gblobs[1].indices),
                        raw_ptr(gblobs[1].indices) + gblobs[1].index_count},
    rparams);

  const auto vertex_bytes =
    (gblobs[0].vertex_count + gblobs[1].vertex_count) * sizeof(vertex_pnt);

  gl::CreateBuffers(1, raw_handle_ptr(_vertices));
  gl::NamedBufferStorage(
    raw_handle(_vertices), vertex_bytes, nullptr, gl::MAP_WRITE_BIT);

  scoped_resource_mapping vbmap{raw_handle(_vertices),
                                gl::MAP_WRITE_BIT |
                                  gl::MAP_INVALIDATE_BUFFER_BIT,
                                vertex_bytes};
  if (!vbmap) {
    XR_LOG_ERR("Failed to map vertex buffer!");
    return;
  }

  const auto index_bytes = gblobs[0].index_count + gblobs[1].index_count;

  _vs = gpu_program_builder{}
          .add_file("shaders/lighting/vs.pointlight.glsl")
          .build<render_stage::e::vertex>();

  if (!_vs)
    return;

  _fs = gpu_program_builder{}
          .add_file("shaders/lighting/fs.pointlight.glsl")
          .build<render_stage::e::fragment>();

  if (!_fs)
    return;

  _pipeline = program_pipeline{[]() {
    GLuint ppl{};
    gl::CreateProgramPipelines(1, &ppl);
    return ppl;
  }()};

  _pipeline.use_vertex_program(_vs).use_fragment_program(_fs);

  gl::CreateSamplers(1, raw_handle_ptr(_sampler));
  const auto smp = raw_handle(_sampler);
  gl::SamplerParameteri(smp, gl::TEXTURE_MIN_FILTER, gl::LINEAR);
  gl::SamplerParameteri(smp, gl::TEXTURE_MAG_FILTER, gl::LINEAR);

  gl::Enable(gl::CULL_FACE);
  gl::Enable(gl::DEPTH_TEST);

  _valid = true;
}

void app::point_light_demo::draw(
  const xray::rendering::draw_context_t& draw_ctx) {
  assert(valid());

  gl::ClearNamedFramebufferfv(
    0, gl::COLOR, 0, color_palette::web::black.components);
  gl::ClearNamedFramebufferfi(0, gl::DEPTH_STENCIL, 0, 1.0f, 0);
}

void app::point_light_demo::update(const float /* delta_ms */) {

  if (_demo_opts.rotate_x) {
  }

  if (_demo_opts.rotate_y) {
  }

  if (_demo_opts.rotate_z) {
  }
}

void app::point_light_demo::event_handler(const xray::ui::window_event& evt) {
  if (evt.type == event_type::key) {
    if (evt.event.key.keycode == key_sym::e::backspace) {
      _demo_opts = decltype(_demo_opts){};
      return;
    }
  }
}

void app::point_light_demo::compose_ui() {
  ImGui::SetNextWindowPos({0, 0}, ImGuiSetCond_Always);
  ImGui::Begin("Options");

  ImGui::Checkbox("Rotate X", &_demo_opts.rotate_x);
  ImGui::Checkbox("Rotate Y", &_demo_opts.rotate_y);
  ImGui::Checkbox("Rotate Z", &_demo_opts.rotate_z);

  ImGui::SliderFloat("Rotation speed", &_demo_opts.rotate_speed, 0.001f, 0.5f);

  ImGui::SliderFloat3(
    "Light direction", _demo_opts.lightpos.components, -1.0f, +1.0f);
  ImGui::SliderFloat(
    "Specular intensity", &_demo_opts.specular_intensity, 1.0f, 256.0f);

  ImGui::Checkbox("Draw surface normals", &_demo_opts.drawnormals);

  ImGui::End();
}
