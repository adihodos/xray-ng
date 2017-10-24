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
#include "xray/base/pod_zero.hpp"
#include "xray/math/constants.hpp"
#include "xray/math/projection.hpp"
#include "xray/math/scalar3.hpp"
#include "xray/math/scalar3_math.hpp"
#include "xray/math/scalar4x4.hpp"
#include "xray/math/scalar4x4_math.hpp"
#include "xray/rendering/colors/color_palettes.hpp"
#include "xray/rendering/colors/rgb_color.hpp"
#include "xray/rendering/draw_context.hpp"
#include "xray/rendering/geometry/geometry_data.hpp"
#include "xray/rendering/geometry/geometry_factory.hpp"
#include "xray/rendering/opengl/gl_handles.hpp"
#include "xray/rendering/opengl/gpu_program.hpp"
#include "xray/rendering/opengl/program_pipeline.hpp"
#include "xray/rendering/opengl/scoped_resource_mapping.hpp"
#include "xray/rendering/vertex_format/vertex_pnt.hpp"
#include <algorithm>
#include <chrono>
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

struct nk_vertex {
  float   position[2];
  float   uv[2];
  nk_byte col[4];
};

using namespace xray::base;
using namespace xray::rendering;
using namespace xray::math;
using namespace xray::ui;
using namespace std;

extern xray::base::app_config* xr_app_config;

struct scoped_nk_input {
public:
  scoped_nk_input(nk_context* ctx) noexcept : _ctx{ctx} {
    assert(_ctx != nullptr);
    nk_input_begin(_ctx);
  }

  ~scoped_nk_input() {
    assert(_ctx != nullptr);
    nk_input_end(_ctx);
  }

private:
  nk_context* _ctx;
};

class user_interface {
public:
  user_interface();

  void ui_event(const xray::ui::window_event& evt);
  void render(const int32_t surface_width, const int32_t surface_height);

  nk_context* ctx() noexcept { return &_renderer.ctx; }

private:
  void upload_font_atlas(const void* pixels, const xray::math::vec2i32& size);

  void setup_render_data();

  static constexpr auto double_click_treshold_ms_low  = 100u;
  static constexpr auto double_click_treshold_ms_high = 200u;
  static constexpr auto max_vertex_buffer_bytes       = 1024u * 1024u;
  static constexpr auto max_index_buffer_bytes        = 512u * 1024u;

  struct render_state {
    nk_buffer                                      cmds;
    nk_draw_null_texture                           null;
    nk_font*                                       font;
    nk_font_atlas                                  font_atlas;
    nk_context                                     ctx;
    xray::rendering::scoped_buffer                 vb;
    xray::rendering::scoped_buffer                 ib;
    xray::rendering::scoped_vertex_array           vao;
    xray::rendering::vertex_program                vs;
    xray::rendering::fragment_program              fs;
    xray::rendering::program_pipeline              pp;
    xray::rendering::scoped_texture                font_tex;
    xray::rendering::scoped_sampler                sampler;
    std::chrono::high_resolution_clock::time_point last_left_click;
    nk_anti_aliasing                               aa{NK_ANTI_ALIASING_ON};
  } _renderer;
};

user_interface::user_interface() {
  _renderer.last_left_click = std::chrono::high_resolution_clock::now();

  nk_buffer_init_default(&_renderer.cmds);
  nk_font_atlas_init_default(&_renderer.font_atlas);

  nk_font_atlas_begin(&_renderer.font_atlas);
  _renderer.font =
    nk_font_atlas_add_default(&_renderer.font_atlas, 13.0f, nullptr);

  vec2i32 atlas_size{};
  auto    image = nk_font_atlas_bake(
    &_renderer.font_atlas, &atlas_size.x, &atlas_size.y, NK_FONT_ATLAS_RGBA32);

  upload_font_atlas(image, atlas_size);
  nk_font_atlas_end(
    &_renderer.font_atlas,
    nk_handle_id(static_cast<int32_t>(raw_handle(_renderer.font_tex))),
    &_renderer.null);

  nk_init_default(&_renderer.ctx, &_renderer.font->handle);

  setup_render_data();
}

void user_interface::upload_font_atlas(const void*                pixels,
                                       const xray::math::vec2i32& size) {
  gl::CreateTextures(gl::TEXTURE_2D, 1, raw_handle_ptr(_renderer.font_tex));
  gl::TextureStorage2D(
    raw_handle(_renderer.font_tex), 1, gl::RGBA8, size.x, size.y);
  gl::TextureSubImage2D(raw_handle(_renderer.font_tex),
                        0,
                        0,
                        0,
                        size.x,
                        size.y,
                        gl::RGBA,
                        gl::UNSIGNED_BYTE,
                        pixels);

  gl::CreateSamplers(1, raw_handle_ptr(_renderer.sampler));
  gl::SamplerParameteri(
    raw_handle(_renderer.sampler), gl::TEXTURE_MIN_FILTER, gl::LINEAR);
  gl::SamplerParameteri(
    raw_handle(_renderer.sampler), gl::TEXTURE_MAG_FILTER, gl::LINEAR);
}

void user_interface::ui_event(const xray::ui::window_event& evt) {
  scoped_nk_input begin_end_input_block{&_renderer.ctx};

  if (!is_input_event(evt)) {
    return;
  }

  auto ctx = &_renderer.ctx;

  //
  // keys
  if (evt.type == event_type::key) {
    const auto ke      = &evt.event.key;
    const auto pressed = ke->type == event_action_type::press;

    if (ke->keycode == key_sym::e::left_shift ||
        ke->keycode == key_sym::e::right_shift) {
      nk_input_key(ctx, NK_KEY_SHIFT, pressed);
    } else if (ke->keycode == key_sym::e::del) {
      nk_input_key(ctx, NK_KEY_DEL, pressed);
    } else if (ke->keycode == key_sym::e::enter) {
      nk_input_key(ctx, NK_KEY_ENTER, pressed);
    } else if (ke->keycode == key_sym::e::tab) {
      nk_input_key(ctx, NK_KEY_TAB, pressed);
    } else if (ke->keycode == key_sym::e::left) {
      nk_input_key(ctx, NK_KEY_LEFT, pressed);
    } else if (ke->keycode == key_sym::e::right) {
      nk_input_key(ctx, NK_KEY_RIGHT, pressed);
    } else if (ke->keycode == key_sym::e::up) {
      nk_input_key(ctx, NK_KEY_UP, pressed);
    } else if (ke->keycode == key_sym::e::down) {
      nk_input_key(ctx, NK_KEY_DOWN, pressed);
    } else if (ke->keycode == key_sym::e::backspace) {
      nk_input_key(ctx, NK_KEY_BACKSPACE, pressed);
    } else if (ke->keycode == key_sym::e::space && !pressed) {
      nk_input_char(ctx, ' ');
    } else if (ke->keycode == key_sym::e::page_up) {
      nk_input_key(ctx, NK_KEY_SCROLL_UP, pressed);
    } else if (ke->keycode == key_sym::e::page_down) {
      nk_input_key(ctx, NK_KEY_SCROLL_DOWN, pressed);
    } else if (ke->keycode == key_sym::e::home) {
      nk_input_key(ctx, NK_KEY_TEXT_START, pressed);
      nk_input_key(ctx, NK_KEY_SCROLL_START, pressed);
    } else if (ke->keycode == key_sym::e::end) {
      nk_input_key(ctx, NK_KEY_TEXT_END, pressed);
      nk_input_key(ctx, NK_KEY_SCROLL_END, pressed);
    } else {
      if (ke->keycode == key_sym::e::key_c && ke->control) {
        nk_input_key(ctx, NK_KEY_COPY, pressed);
      } else if (ke->keycode == key_sym::e::key_v && ke->control) {
        nk_input_key(ctx, NK_KEY_PASTE, pressed);
      } else if (ke->keycode == key_sym::e::key_x && ke->control) {
        nk_input_key(ctx, NK_KEY_CUT, pressed);
      } else if (ke->keycode == key_sym::e::key_z && ke->control) {
        nk_input_key(ctx, NK_KEY_TEXT_UNDO, pressed);
      } else if (ke->keycode == key_sym::e::key_r && ke->control) {
        nk_input_key(ctx, NK_KEY_TEXT_REDO, pressed);
      } else {
        if (pressed) {
          nk_input_glyph(ctx, ke->name);
        }
      }
    }

    return;
  }

  //
  // mouse buttons
  if (evt.type == event_type::mouse_button) {
    const auto me      = &evt.event.button;
    const auto pressed = me->type == event_action_type::press;

    if (me->button == mouse_button::button1) {
      // if (pressed) {
      //  const auto ts = std::chrono::high_resolution_clock::now();
      //  const auto elapsed_ms =
      //    std::chrono::duration_cast<std::chrono::milliseconds>(
      //      ts - _renderer.last_left_click)
      //      .count();

      //  if (elapsed_ms >= double_click_treshold_ms_low &&
      //      elapsed_ms <= double_click_treshold_ms_high) {
      //    XR_LOG_INFO("Double click");
      //    nk_input_button(
      //      ctx, NK_BUTTON_DOUBLE, me->pointer_x, me->pointer_y, nk_true);
      //  }

      //  _renderer.last_left_click = ts;
      //} else {
      //  nk_input_button(
      //    ctx, NK_BUTTON_DOUBLE, me->pointer_x, me->pointer_y, nk_false);
      //}
      XR_DBG_MSG("Mouse1 {}!", pressed ? "down" : "up");
      nk_input_button(
        ctx, NK_BUTTON_LEFT, me->pointer_x, me->pointer_y, pressed);
    } else if (me->button == mouse_button::button2) {
      XR_DBG_MSG("Button 2");
      nk_input_button(
        ctx, NK_BUTTON_MIDDLE, me->pointer_x, me->pointer_y, pressed);
    } else if (me->button == mouse_button::button3) {
      XR_DBG_MSG("Button 3");
      nk_input_button(
        ctx, NK_BUTTON_RIGHT, me->pointer_x, me->pointer_y, pressed);
    }

    return;
  }

  //
  // mouse scroll
  if (evt.type == event_type::mouse_wheel) {
    const auto mw = &evt.event.wheel;
    if (mw->delta > 0) {
      nk_input_scroll(ctx, nk_vec2(0.0f, 1.0f));
    } else {
      nk_input_scroll(ctx, nk_vec2(0.0f, -1.0f));
    }

    return;
  }

  //
  // mouse movement
  if (evt.type == event_type::mouse_motion) {
    const auto mm = &evt.event.motion;
    nk_input_motion(ctx, mm->pointer_x, mm->pointer_y);
    return;
  }
}

void user_interface::setup_render_data() {
  static constexpr const char* vertex_shader =
    "#version 450 core\n"
    "layout (row_major) uniform;\n"
    "layout (binding = 0) uniform Transforms { \n"
    "   mat4 ProjMtx;\n"
    "};\n"
    "layout (location = 0) in vec2 Position;\n"
    "layout (location = 1) in vec2 TexCoord;\n"
    "layout (location = 2) in vec4 Color;\n"
    "out gl_PerVertex {\n"
    "   vec4 gl_Position;\n"
    "};\n"
    "out vec2 Frag_UV;\n"
    "out vec4 Frag_Color;\n"
    "void main() {\n"
    "   Frag_UV = TexCoord;\n"
    "   Frag_Color = Color;\n"
    "   gl_Position = ProjMtx * vec4(Position.xy, 0, 1);\n"
    "}\n";

  static constexpr const char* fragment_shader =
    "#version 450 core\n"
    "layout (binding = 0) uniform sampler2D Texture;\n"
    "in vec2 Frag_UV;\n"
    "in vec4 Frag_Color;\n"
    "out vec4 Out_Color;\n"
    "void main(){\n"
    "   Out_Color = Frag_Color * texture(Texture, Frag_UV.st);\n"
    "}\n";

  _renderer.vs = gpu_program_builder{}
                   .add_string(vertex_shader)
                   .build<render_stage::e::vertex>();

  _renderer.fs = gpu_program_builder{}
                   .add_string(fragment_shader)
                   .build<render_stage::e::fragment>();

  _renderer.pp = program_pipeline{[]() {
    GLuint pp{};
    gl::CreateProgramPipelines(1, &pp);
    return pp;
  }()};

  _renderer.pp.use_vertex_program(_renderer.vs);
  _renderer.pp.use_fragment_program(_renderer.fs);

  gl::CreateBuffers(1, raw_handle_ptr(_renderer.vb));
  gl::NamedBufferStorage(raw_handle(_renderer.vb),
                         max_vertex_buffer_bytes,
                         nullptr,
                         gl::MAP_WRITE_BIT);

  gl::CreateBuffers(1, raw_handle_ptr(_renderer.ib));
  gl::NamedBufferStorage(raw_handle(_renderer.ib),
                         max_index_buffer_bytes,
                         nullptr,
                         gl::MAP_WRITE_BIT);

  gl::CreateVertexArrays(1, raw_handle_ptr(_renderer.vao));
  const auto vao = raw_handle(_renderer.vao);

  gl::VertexArrayVertexBuffer(
    vao, 0, raw_handle(_renderer.vb), 0, sizeof(nk_vertex));
  gl::VertexArrayElementBuffer(vao, raw_handle(_renderer.ib));

  gl::VertexArrayAttribFormat(
    vao, 0, 2, gl::FLOAT, gl::FALSE_, XR_U32_OFFSETOF(nk_vertex, position));
  gl::VertexArrayAttribFormat(
    vao, 1, 2, gl::FLOAT, gl::FALSE_, XR_U32_OFFSETOF(nk_vertex, uv));
  gl::VertexArrayAttribFormat(
    vao, 2, 4, gl::UNSIGNED_BYTE, gl::TRUE_, XR_U32_OFFSETOF(nk_vertex, col));

  gl::VertexArrayAttribBinding(vao, 0, 0);
  gl::VertexArrayAttribBinding(vao, 1, 0);
  gl::VertexArrayAttribBinding(vao, 2, 0);

  gl::EnableVertexArrayAttrib(vao, 0);
  gl::EnableVertexArrayAttrib(vao, 1);
  gl::EnableVertexArrayAttrib(vao, 2);
}

void user_interface::render(const int32_t surface_width,
                            const int32_t surface_height) {
  gl::ViewportIndexedf(0,
                       0.0f,
                       0.0f,
                       static_cast<float>(surface_width),
                       static_cast<float>(surface_height));

  //
  // fill vertex and index buffers
  {
    //      const nk_draw_command* cmd{};
    scoped_resource_mapping vbmap{raw_handle(_renderer.vb),
                                  gl::MAP_WRITE_BIT |
                                    gl::MAP_INVALIDATE_BUFFER_BIT,
                                  max_vertex_buffer_bytes};

    if (!vbmap) {
      XR_LOG_ERR("Failed to map vertex buffer!");
      return;
    }

    scoped_resource_mapping ibmap{raw_handle(_renderer.ib),
                                  gl::MAP_WRITE_BIT |
                                    gl::MAP_INVALIDATE_BUFFER_BIT,
                                  max_index_buffer_bytes};

    if (!ibmap) {
      XR_LOG_ERR("Failed to map index buffer!");
      return;
    }

    static const struct nk_draw_vertex_layout_element vertex_layout[] = {
      {NK_VERTEX_POSITION,
       NK_FORMAT_FLOAT,
       XR_U32_OFFSETOF(struct nk_vertex, position)},
      {NK_VERTEX_TEXCOORD,
       NK_FORMAT_FLOAT,
       XR_U32_OFFSETOF(struct nk_vertex, uv)},
      {NK_VERTEX_COLOR,
       NK_FORMAT_R8G8B8A8,
       XR_U32_OFFSETOF(struct nk_vertex, col)},
      {NK_VERTEX_LAYOUT_END}};

    pod_zero<nk_convert_config> config;
    config.vertex_layout        = vertex_layout;
    config.vertex_size          = sizeof(nk_vertex);
    config.vertex_alignment     = NK_ALIGNOF(nk_vertex);
    config.null                 = _renderer.null;
    config.circle_segment_count = 22;
    config.curve_segment_count  = 22;
    config.arc_segment_count    = 22;
    config.global_alpha         = 1.0f;
    config.shape_AA             = _renderer.aa;
    config.line_AA              = _renderer.aa;

    {
      nk_buffer vbuff;
      nk_buffer_init_fixed(&vbuff, vbmap.memory(), max_vertex_buffer_bytes);

      nk_buffer ebuff;
      nk_buffer_init_fixed(&ebuff, ibmap.memory(), max_index_buffer_bytes);

      nk_convert(&_renderer.ctx, &_renderer.cmds, &vbuff, &ebuff, &config);
    }
  }

  struct opengl_state_save_restore {
    GLint last_blend_src;
    GLint last_blend_dst;
    GLint last_blend_eq_rgb;
    GLint last_blend_eq_alpha;
    GLint last_viewport[4];
    GLint blend_enabled;
    GLint cullface_enabled;
    GLint depth_enabled;
    GLint scissors_enabled;

    opengl_state_save_restore() {
      gl::GetIntegerv(gl::BLEND_SRC, &last_blend_src);
      gl::GetIntegerv(gl::BLEND_DST, &last_blend_dst);
      gl::GetIntegerv(gl::BLEND_EQUATION_RGB, &last_blend_eq_rgb);
      gl::GetIntegerv(gl::BLEND_EQUATION_ALPHA, &last_blend_eq_alpha);
      gl::GetIntegerv(gl::VIEWPORT, last_viewport);
      blend_enabled    = gl::IsEnabled(gl::BLEND);
      cullface_enabled = gl::IsEnabled(gl::CULL_FACE);
      depth_enabled    = gl::IsEnabled(gl::DEPTH_TEST);
      scissors_enabled = gl::IsEnabled(gl::SCISSOR_TEST);
    }

    ~opengl_state_save_restore() {
      gl::BlendEquationSeparate(last_blend_eq_rgb, last_blend_eq_alpha);
      gl::BlendFunc(last_blend_src, last_blend_dst);
      blend_enabled ? gl::Enable(gl::BLEND) : gl::Disable(gl::BLEND);
      cullface_enabled ? gl::Enable(gl::CULL_FACE) : gl::Disable(gl::CULL_FACE);
      depth_enabled ? gl::Enable(gl::DEPTH_TEST) : gl::Disable(gl::DEPTH_TEST);
      scissors_enabled ? gl::Enable(gl::SCISSOR_TEST)
                       : gl::Disable(gl::SCISSOR_TEST);
      gl::Viewport(
        last_viewport[0], last_viewport[1], last_viewport[2], last_viewport[3]);
    }
  } state_save_restore{};

  gl::Enable(gl::BLEND);
  gl::BlendEquation(gl::FUNC_ADD);
  gl::BlendFunc(gl::SRC_ALPHA, gl::ONE_MINUS_SRC_ALPHA);
  gl::Disable(gl::CULL_FACE);
  gl::Disable(gl::DEPTH_TEST);
  gl::Enable(gl::SCISSOR_TEST);
  gl::BindSampler(0, raw_handle(_renderer.sampler));

  gl::BindVertexArray(raw_handle(_renderer.vao));

  auto mk_proj = [](const float left,
                    const float right,
                    const float top,
                    const float bottom,
                    const float znear,
                    const float zfar) {
    mat4f result{mat4f::stdc::identity};

    result.a00 = 2.0f / (right - left);
    result.a03 = -(right + left) / (right - left);

    result.a11 = 2.0f / (top - bottom);
    result.a13 = -(top + bottom) / (top - bottom);

    result.a22 = -2.0f / (zfar - znear);
    result.a23 = -(zfar + znear) / (zfar - znear);

    return result;
  };

  auto mkproj =
    [&](const float w, const float h, const float zn, const float zf) {
      return mk_proj(-w * 0.5f, +w * 0.5f, -h * 0.5f, +h * 0.5f, zn, zf);
    };

  struct {
    mat4f projection_mtx;
  } const transforms = {mk_proj(0.0f,
                                static_cast<float>(surface_width),
                                0.0f,
                                static_cast<float>(surface_height),
                                -1.0f,
                                +1.0f)};

  _renderer.vs.set_uniform_block("Transforms", transforms);

  _renderer.pp.use();

  const nk_draw_command* cmd{};
  const nk_draw_index*   offset{};
  nk_draw_foreach(cmd, &_renderer.ctx, &_renderer.cmds) {
    if (!cmd->elem_count) {
      continue;
    }

    gl::BindTextureUnit(0, static_cast<GLuint>(cmd->texture.id));
    gl::Scissor(
      (GLint)(cmd->clip_rect.x),
      (GLint)((surface_height - (GLint)(cmd->clip_rect.y + cmd->clip_rect.h))),
      (GLint)(cmd->clip_rect.w),
      (GLint)(cmd->clip_rect.h));
    gl::DrawElements(
      gl::TRIANGLES, cmd->elem_count, gl::UNSIGNED_SHORT, offset);

    offset += cmd->elem_count;
  }

  nk_clear(&_renderer.ctx);
}

user_interface* ui{};

app::point_light_demo::point_light_demo(const init_context_t* /*init_ctx*/) {
  init();
  ui = new user_interface();
}

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

  const auto vertex_bytes = static_cast<uint32_t>(
    (gblobs[0].vertex_count + gblobs[1].vertex_count) * sizeof(vertex_pnt));

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

  //_vs = gpu_program_builder{}
  //        .add_file("shaders/lighting/vs.pointlight.glsl")
  //        .build<render_stage::e::vertex>();

  // if (!_vs)
  //  return;

  //_fs = gpu_program_builder{}
  //        .add_file("shaders/lighting/fs.pointlight.glsl")
  //        .build<render_stage::e::fragment>();

  // if (!_fs)
  //  return;

  //_pipeline = program_pipeline{[]() {
  //  GLuint ppl{};
  //  gl::CreateProgramPipelines(1, &ppl);
  //  return ppl;
  //}()};

  //_pipeline.use_vertex_program(_vs).use_fragment_program(_fs);

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

  auto            ctx        = ui->ctx();
  static nk_color background = nk_rgb(28, 48, 62);

  if (nk_begin(ctx,
               "Demo",
               nk_rect(50, 50, 200, 200),
               NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_SCALABLE |
                 NK_WINDOW_CLOSABLE | NK_WINDOW_MINIMIZABLE |
                 NK_WINDOW_TITLE)) {
    enum { EASY, HARD };
    static int op       = EASY;
    static int property = 20;

    nk_layout_row_static(ctx, 30, 80, 1);
    if (nk_button_label(ctx, "button"))
      fprintf(stdout, "button pressed\n");
    nk_layout_row_dynamic(ctx, 30, 2);
    if (nk_option_label(ctx, "easy", op == EASY))
      op = EASY;
    if (nk_option_label(ctx, "hard", op == HARD))
      op = HARD;
    nk_layout_row_dynamic(ctx, 25, 1);
    nk_property_int(ctx, "Compression:", 0, &property, 100, 10, 1);

    nk_layout_row_dynamic(ctx, 20, 1);
    nk_label(ctx, "background:", NK_TEXT_LEFT);
    nk_layout_row_dynamic(ctx, 25, 1);
    if (nk_combo_begin_color(
          ctx, background, nk_vec2(nk_widget_width(ctx), 400))) {
      nk_layout_row_dynamic(ctx, 120, 1);
      background = nk_color_picker(ctx, background, NK_RGBA);
      nk_layout_row_dynamic(ctx, 25, 1);
      background.r =
        (nk_byte) nk_propertyi(ctx, "#R:", 0, background.r, 255, 1, 1);
      background.g =
        (nk_byte) nk_propertyi(ctx, "#G:", 0, background.g, 255, 1, 1);
      background.b =
        (nk_byte) nk_propertyi(ctx, "#B:", 0, background.b, 255, 1, 1);
      background.a =
        (nk_byte) nk_propertyi(ctx, "#A:", 0, background.a, 255, 1, 1);
      nk_combo_end(ctx);
    }
  }
  nk_end(ctx);

  ui->render(draw_ctx.window_width, draw_ctx.window_height);
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
  ui->ui_event(evt);
}

void app::point_light_demo::compose_ui() {}
