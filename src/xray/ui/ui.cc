//
// Copyright (c) 2011, 2012, 2013, 2014, 2015, 2016 Adrian Hodos
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

#include "xray/ui/ui.hpp"
#include "xray/base/debug_output.hpp"
#include "xray/base/pod_zero.hpp"
#include "xray/math/projection.hpp"
#include "xray/math/scalar4x4.hpp"
#include "xray/rendering/opengl/scoped_resource_mapping.hpp"
#include "xray/ui/events.hpp"
#include <algorithm>
#include <platformstl/filesystem/memory_mapped_file.hpp>
#include <platformstl/filesystem/path.hpp>
#include <span.h>

#define NK_IMPLEMENTATION
#include <nuklear/nuklear.h>

using namespace xray::math;
using namespace xray::rendering;
using namespace xray::base;
using namespace std;

struct nk_vertex {
  float   position[2];
  float   uv[2];
  nk_byte col[4];
};

void xray::ui::user_interface::init(
  const xray::ui::font_load_info* fonts_to_load, const size_t num_fonts) {
  _renderer.last_left_click = std::chrono::high_resolution_clock::now();

  nk_buffer_init_default(&_renderer.cmds);

  nk_font_atlas_init_default(&_renderer.font_atlas);

  nk_font_atlas_begin(&_renderer.font_atlas);
  _renderer.fonts["default"] =
    nk_font_atlas_add_default(&_renderer.font_atlas, 13.0f, nullptr);

  if (fonts_to_load) {
    const auto font_span = gsl::span<const font_load_info>{
      fonts_to_load, fonts_to_load + static_cast<ptrdiff_t>(num_fonts)};

    for_each(
      begin(font_span), end(font_span), [this](const font_load_info& fli) {
        try {
          platformstl::memory_mapped_file mapped_font_file{
            fli.font_file_path.c_str()};

          const auto font_cfg = nk_font_config(fli.pixel_size);
          auto       font     = nk_font_atlas_add_from_memory(
            &_renderer.font_atlas,
            const_cast<void*>(mapped_font_file.memory()),
            mapped_font_file.size(),
            fli.pixel_size,
            &font_cfg);

          if (!font) {
            XR_DBG_MSG("NK failed to parse font {}", fli.font_file_path);
            return;
          }

          platformstl::path_a fpath{fli.font_file_path};
          fpath.pop_ext();
          XR_DBG_MSG("Loaded font {}", fpath.get_file());

          _renderer.fonts[fpath.get_file()] = font;
        } catch (const std::exception&) {
          return;
        }
      });
  }

  vec2i32 atlas_size{};
  auto    image = nk_font_atlas_bake(
    &_renderer.font_atlas, &atlas_size.x, &atlas_size.y, NK_FONT_ATLAS_RGBA32);

  upload_font_atlas(image, atlas_size);
  nk_font_atlas_end(
    &_renderer.font_atlas,
    nk_handle_id(static_cast<int32_t>(raw_handle(_renderer.font_tex))),
    &_renderer.null);

  nk_init_default(&_renderer.ctx, &_renderer.fonts["default"]->handle);
  setup_render_data();
}

xray::ui::user_interface::~user_interface() {
  nk_font_atlas_clear(&_renderer.font_atlas);
  nk_free(&_renderer.ctx);
}

void xray::ui::user_interface::upload_font_atlas(
  const void* pixels, const xray::math::vec2i32& size) {
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

void xray::ui::user_interface::ui_event(const xray::ui::window_event& evt) {
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
      if (pressed) {
        const auto ts = std::chrono::high_resolution_clock::now();
        const auto elapsed_ms =
          std::chrono::duration_cast<std::chrono::milliseconds>(
            ts - _renderer.last_left_click)
            .count();

        if (elapsed_ms >= double_click_treshold_ms_low &&
            elapsed_ms <= double_click_treshold_ms_high) {
          XR_LOG_INFO("Double click");
          nk_input_button(
            ctx, NK_BUTTON_DOUBLE, me->pointer_x, me->pointer_y, nk_true);
        }

        _renderer.last_left_click = ts;
      } else {
        nk_input_button(
          ctx, NK_BUTTON_DOUBLE, me->pointer_x, me->pointer_y, nk_false);
      }

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
      nk_input_scroll(ctx, nk_vec2(0.0f, -1.0f));
    } else {
      nk_input_scroll(ctx, nk_vec2(0.0f, +1.0f));
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

void xray::ui::user_interface::setup_render_data() {
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

  _renderer.pp.use_vertex_program(_renderer.vs)
    .use_fragment_program(_renderer.fs);

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

void xray::ui::user_interface::render(const int32_t surface_width,
                                      const int32_t surface_height) {

  struct scoped_clear_ctx {
  public:
    scoped_clear_ctx(nk_context* ctx) noexcept : _ctx{ctx} {}

    ~scoped_clear_ctx() noexcept { nk_clear(_ctx); }

  private:
    nk_context* _ctx;
  };

  scoped_clear_ctx clear_nk_context_on_exit{&_renderer.ctx};

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

  struct {
    mat4f projection_mtx;
  } const transforms = {
    projections_rh::orthographic(0.0f,
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
}

nk_font* xray::ui::user_interface::find_font(const char* name) noexcept {
  auto font_entry = _renderer.fonts.find(name);
  if (font_entry == std::end(_renderer.fonts)) {
    XR_DBG_MSG("Trying to set non existent font {}", name);
  }

  return font_entry->second;
}
