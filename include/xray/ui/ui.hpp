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

#pragma once

/// \file   input_event.hpp

#include "xray/xray.hpp"
#include "unordered_map"
#include <cassert>
#include <chrono>
#include <cstdint>
#include <string>

#include "xray/base/fast_delegate.hpp"
#include "xray/math/scalar2.hpp"
#include "xray/math/scalar3.hpp"
#include "xray/math/scalar4.hpp"
#include "xray/rendering/colors/rgb_color.hpp"
#if defined(XRAY_RENDERER_DIRECTX)
#include "xray/base/windows/com_ptr.hpp"
#include "xray/rendering/directx/pixel_shader.hpp"
#include "xray/rendering/directx/vertex_shader.hpp"
#include <d3d11.h>
#else
#include "xray/rendering/opengl/gl_handles.hpp"
#include "xray/rendering/opengl/gpu_program.hpp"
#include "xray/rendering/opengl/program_pipeline.hpp"
#endif

#include "xray/ui/nk_config.hpp"
#include <nuklear/nuklear.h>

namespace xray {
namespace ui {

/// \addtogroup __GroupXrayUI
/// @{

struct window_event;

class user_interface {
public:
  user_interface() { init(nullptr, 0, 13.0f); }

  user_interface(const char** fonts,
                 const size_t num_fonts,
                 const float  pixel_size = 13.0f) {
    init(fonts, num_fonts, pixel_size);
  }

  ~user_interface();

  void ui_event(const xray::ui::window_event& evt);
  void render(const int32_t surface_width, const int32_t surface_height);

  nk_context* ctx() noexcept { return &_renderer.ctx; }

  void input_begin() noexcept { nk_input_begin(&_renderer.ctx); }

  void input_end() noexcept { nk_input_end(&_renderer.ctx); }

  void set_font(const char* name) noexcept;

  bool wants_input() noexcept {
    return nk_item_is_any_active(&_renderer.ctx) == nk_true;
  }

  struct _combobox {
  private:
    friend class user_interface;
    user_interface* _owner;
    _combobox(user_interface* owner) : _owner{owner} {}

  public:
    int32_t combo(const char**             items,
                  size_t                   count,
                  int32_t                  selected,
                  int32_t                  item_height,
                  const xray::math::vec2f& size) {
      return nk_combo(_owner->ctx(),
                      items,
                      static_cast<int32_t>(count),
                      selected,
                      item_height,
                      nk_vec2v(size.components));
    }

    int32_t combo_separator(const char* items_separated_by_separator,
                            int32_t     separator,
                            int32_t     selected,
                            size_t      count,
                            int32_t     item_height,
                            const xray::math::vec2f& size) {
      return nk_combo_separator(_owner->ctx(),
                                items_separated_by_separator,
                                separator,
                                selected,
                                static_cast<int32_t>(count),
                                item_height,
                                nk_vec2v(size.components));
    }

    int32_t combo_string(const char*              items_separated_by_zeros,
                         int32_t                  selected,
                         size_t                   count,
                         int                      item_height,
                         const xray::math::vec2f& size) {
      return nk_combo_string(_owner->ctx(),
                             items_separated_by_zeros,
                             selected,
                             static_cast<int32_t>(count),
                             item_height,
                             nk_vec2v(size.components));
    }

    int32_t combo_callback(void (*item_getter)(void*, int32_t, const char**),
                           void*                    userdata,
                           int32_t                  selected,
                           size_t                   count,
                           int32_t                  item_height,
                           const xray::math::vec2f& size) {
      return nk_combo_callback(_owner->ctx(),
                               item_getter,
                               userdata,
                               selected,
                               static_cast<int32_t>(count),
                               item_height,
                               nk_vec2v(size.components));
    }

    void combobox(const char**             items,
                  size_t                   count,
                  int32_t*                 selected,
                  int32_t                  item_height,
                  const xray::math::vec2f& size) {
      return nk_combobox(_owner->ctx(),
                         items,
                         static_cast<int32_t>(count),
                         selected,
                         item_height,
                         nk_vec2v(size.components));
    }

    void combobox_string(const char*              items_separated_by_zeros,
                         int32_t*                 selected,
                         size_t                   count,
                         int32_t                  item_height,
                         const xray::math::vec2f& size) {
      return nk_combobox_string(_owner->ctx(),
                                items_separated_by_zeros,
                                selected,
                                static_cast<int32_t>(count),
                                item_height,
                                nk_vec2v(size.components));
    }

    void combobox_separator(struct nk_context*,
                            const char* items_separated_by_separator,
                            int32_t     separator,
                            int32_t*    selected,
                            size_t      count,
                            int32_t     item_height,
                            const xray::math::vec2f& size) {
      return nk_combobox_separator(_owner->ctx(),
                                   items_separated_by_separator,
                                   separator,
                                   selected,
                                   static_cast<int32_t>(count),
                                   item_height,
                                   nk_vec2v(size.components));
    }

    void combobox_callback(void (*item_getter)(void*, int32_t, const char**),
                           void*                    userdata,
                           int32_t*                 selected,
                           int32_t                  count,
                           int32_t                  item_height,
                           const xray::math::vec2f& size) {
      return nk_combobox_callback(_owner->ctx(),
                                  item_getter,
                                  userdata,
                                  selected,
                                  static_cast<int32_t>(count),
                                  item_height,
                                  nk_vec2v(size.components));
    }
  } combo{this};

  struct _abstract_combo {
  private:
    friend class user_interface;
    _abstract_combo(user_interface* owner) : _owner{owner} {}

  public:
    int begin_text(const char* selected, const xray::math::vec2f& size) {
      return nk_combo_begin_text(
        _owner->ctx(), selected, strlen(selected), nk_vec2v(size.components));
    }

    int begin_label(const char* selected, const xray::math::vec2f& size) {
      return nk_combo_begin_label(
        _owner->ctx(), selected, nk_vec2v(size.components));
    }

    int begin_color(const xray::rendering::rgb_color& clr,
                    const xray::math::vec2f           size) {
      return nk_combo_begin_color(
        _owner->ctx(), nk_rgba_fv(clr.components), nk_vec2v(size.components));
    }

    int begin_symbol(enum nk_symbol_type stype, const xray::math::vec2f& size) {
      return nk_combo_begin_symbol(
        _owner->ctx(), stype, nk_vec2v(size.components));
    }

    int begin_symbol_label(const char*              selected,
                           enum nk_symbol_type      stype,
                           const xray::math::vec2f& size) {
      return nk_combo_begin_symbol_label(
        _owner->ctx(), selected, stype, nk_vec2v(size.components));
    }

    int begin_symbol_text(const char*              selected,
                          enum nk_symbol_type      stype,
                          const xray::math::vec2f& size) {
      return nk_combo_begin_symbol_text(_owner->ctx(),
                                        selected,
                                        strlen(selected),
                                        stype,
                                        nk_vec2v(size.components));
    }

    int begin_image(struct nk_image img, const xray::math::vec2f& size) {
      return nk_combo_begin_image(
        _owner->ctx(), img, nk_vec2v(size.components));
    }

    int begin_image_label(const char*              selected,
                          struct nk_image          img,
                          const xray::math::vec2f& size) {
      return nk_combo_begin_image_label(
        _owner->ctx(), selected, img, nk_vec2v(size.components));
    }

    int begin_image_text(const char*              selected,
                         struct nk_image          img,
                         const xray::math::vec2f& size) {
      return nk_combo_begin_image_text(_owner->ctx(),
                                       selected,
                                       strlen(selected),
                                       img,
                                       nk_vec2v(size.components));
    }

    int item_label(const char* txt, nk_flags alignment) {
      return nk_combo_item_label(_owner->ctx(), txt, alignment);
    }

    int item_text(const char* txt, nk_flags alignment) {
      return nk_combo_item_text(_owner->ctx(), txt, strlen(txt), alignment);
    }

    int
    item_image_label(struct nk_image img, const char* txt, nk_flags alignment) {
      return nk_combo_item_image_label(_owner->ctx(), img, txt, alignment);
    }

    int
    item_image_text(struct nk_image img, const char* txt, nk_flags alignment) {
      return nk_combo_item_image_text(
        _owner->ctx(), img, txt, strlen(txt), alignment);
    }

    int item_symbol_label(enum nk_symbol_type stype,
                          const char*         txt,
                          nk_flags            alignment) {
      return nk_combo_item_symbol_label(_owner->ctx(), stype, txt, alignment);
    }

    int item_symbol_text(enum nk_symbol_type stype,
                         const char*         txt,
                         nk_flags            alignment) {
      return nk_combo_item_symbol_text(
        _owner->ctx(), stype, txt, strlen(txt), alignment);
    }

    void combo_close() { nk_combo_close(_owner->ctx()); }

    void end() { nk_combo_end(_owner->ctx()); }

  private:
    user_interface* _owner;
  } abstract_combo{this};

  float widget_width() noexcept { return nk_widget_width(ctx()); }

private:
  void init(const char** fonts, const size_t num_fonts, const float pixel_size);

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
    std::unordered_map<std::string, nk_font*>      fonts;
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

  XRAY_NO_COPY(user_interface);
};

/// @}

} // namespace ui
} // namespace xray
