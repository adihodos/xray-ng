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

#include "unordered_map"
#include "xray/xray.hpp"
#include <cassert>
#include <chrono>
#include <cstdint>
#include <string>

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
#include <fmt/format.h>
#include <nuklear/nuklear.h>

#include "xray/ui/events.hpp"

namespace xray {
namespace ui {

/// \addtogroup __GroupXrayUI
/// @{

struct font_load_info
{
    std::string font_file_path;
    float pixel_size;
};

class user_interface
{
  private:
    static nk_buttons translate_mouse_button(const xray::ui::mouse_button btn)
    {
        switch (btn) {
            case mouse_button::button1:
                return NK_BUTTON_LEFT;
                break;

            case mouse_button::button2:
                return NK_BUTTON_MIDDLE;
                break;

            case mouse_button::button3:
                return NK_BUTTON_RIGHT;
                break;

            default:
                break;
        }

        return NK_BUTTON_MAX;
    }

    nk_font* find_font(const char* name) noexcept;

  public:
    user_interface() { init(nullptr, 0); }

    user_interface(const font_load_info* fonts_to_load, const size_t num_fonts) { init(fonts_to_load, num_fonts); }

    ~user_interface();

    void ui_event(const xray::ui::window_event& evt);
    void render(const int32_t surface_width, const int32_t surface_height);

    nk_context* ctx() noexcept { return &_renderer.ctx; }

    struct _input
    {
      private:
        friend class user_interface;
        user_interface* _owner;
        _input(user_interface* owner)
            : _owner{ owner }
        {
        }

      public:
        void begin() { nk_input_begin(_owner->ctx()); }

        void end() { nk_input_end(_owner->ctx()); }

        bool wants() noexcept { return nk_item_is_any_active(_owner->ctx()) == nk_true; }

    } input{ this };

    struct _window
    {
      private:
        friend class user_interface;
        user_interface* _owner;
        _window(user_interface* owner)
            : _owner{ owner }
        {
        }

      public:
        int32_t begin(const char* title,
                      const float x,
                      const float y,
                      const float width,
                      const float height,
                      const nk_flags flags)
        {
            return nk_begin(_owner->ctx(), title, nk_rect(x, y, width, height), flags);
        }

        int32_t begin_titled(const char* name,
                             const char* title,
                             const float x,
                             const float y,
                             const float width,
                             const float height,
                             const nk_flags flags)
        {
            return nk_begin_titled(_owner->ctx(), name, title, nk_rect(x, y, width, height), flags);
        }

        void end() { nk_end(_owner->ctx()); }

    } window{ this };

    struct _layout
    {
      private:
        friend class user_interface;
        user_interface* _owner;
        _layout(user_interface* owner)
            : _owner{ owner }
        {
        }

      public:
        void set_min_row_height(float height) { nk_layout_set_min_row_height(_owner->ctx(), height); }

        void reset_min_row_height() { nk_layout_reset_min_row_height(_owner->ctx()); }

        float ratio_from_pixel(float pixel_width) { return nk_layout_ratio_from_pixel(_owner->ctx(), pixel_width); }

        void row_dynamic(float height, int32_t cols) { nk_layout_row_dynamic(_owner->ctx(), height, cols); }

        void row_static(float height, int item_width, int cols)
        {
            nk_layout_row_static(_owner->ctx(), height, item_width, cols);
        }

        void row_begin(enum nk_layout_format fmt, float row_height, int32_t cols)
        {
            nk_layout_row_begin(_owner->ctx(), fmt, row_height, cols);
        }

        void row_push(float value) { nk_layout_row_push(_owner->ctx(), value); }

        void row_end() { nk_layout_row_end(_owner->ctx()); }

        void row(enum nk_layout_format fmt, float height, int32_t cols, const float* ratio)
        {
            nk_layout_row(_owner->ctx(), fmt, height, cols, ratio);
        }

        void row_template_begin(float row_height) { nk_layout_row_template_begin(_owner->ctx(), row_height); }

        void row_template_push_dynamic() { nk_layout_row_template_push_dynamic(_owner->ctx()); }

        void row_template_push_variable(float min_width)
        {
            nk_layout_row_template_push_variable(_owner->ctx(), min_width);
        }

        void row_template_push_static(float width) { nk_layout_row_template_push_static(_owner->ctx(), width); }

        void row_template_end() { nk_layout_row_template_end(_owner->ctx()); }

        void space_begin(enum nk_layout_format fmt, float height, int widget_count)
        {
            nk_layout_space_begin(_owner->ctx(), fmt, height, widget_count);
        }

        void space_push(float x, float y, float w, float h)
        {
            nk_layout_space_push(_owner->ctx(), nk_rect(x, y, w, h));
        }

        void space_end() { nk_layout_space_end(_owner->ctx()); }

    } layout{ this };

    struct _group
    {
      private:
        friend class user_interface;
        user_interface* _owner;
        _group(user_interface* owner)
            : _owner{ owner }
        {
        }

        int32_t begin(const char* title, const nk_flags flags) { return nk_group_begin(_owner->ctx(), title, flags); }

        int32_t scrolled_offset_begin(uint32_t* x_offset, uint32_t* y_offset, const char* title, const nk_flags flags)
        {
            return nk_group_scrolled_offset_begin(_owner->ctx(), x_offset, y_offset, title, flags);
        }

        int32_t scrolled_begin(xray::math::vec2ui32* scroll, const char* title, const nk_flags flags)
        {
            return nk_group_scrolled_begin(_owner->ctx(), reinterpret_cast<struct nk_scroll*>(scroll), title, flags);
        }

        void scrolled_end() { nk_group_scrolled_end(_owner->ctx()); }

        void end() { nk_group_end(_owner->ctx()); }

    } group{ this };

    struct _text
    {
      private:
        friend class user_interface;
        user_interface* _owner;
        _text(user_interface* owner)
            : _owner{ owner }
        {
        }

      public:
        void text(const char* txt, const nk_flags flags) { nk_text(_owner->ctx(), txt, strlen(txt), flags); }

        void text_colored(const char* txt, const nk_flags flags, const xray::rendering::rgb_color& color)
        {
            nk_text_colored(_owner->ctx(), txt, strlen(txt), flags, nk_rgba_fv(color.components));
        }

        void text_wrap(const char* txt) { nk_text_wrap(_owner->ctx(), txt, strlen(txt)); }

        void text_wrap_colored(const char* txt, const xray::rendering::rgb_color& color)
        {
            nk_text_wrap_colored(_owner->ctx(), txt, strlen(txt), nk_rgba_fv(color.components));
        }

        void label(const char* txt, const nk_flags align) { nk_label(_owner->ctx(), txt, align); }

        void label_colored(const char* txt, const nk_flags align, const xray::rendering::rgb_color& clr)
        {
            nk_label_colored(_owner->ctx(), txt, align, nk_rgba_fv(clr.components));
        }

        void label_wrap(const char* txt) { nk_label_wrap(_owner->ctx(), txt); }

        void label_colored_wrap(const char* txt, const xray::rendering::rgb_color& color)
        {
            nk_label_colored_wrap(_owner->ctx(), txt, nk_rgba_fv(color.components));
        }

        void image(struct nk_image img) { nk_image(_owner->ctx(), img); }

        template<typename... Args>
        void labelf(const nk_flags flags, const char* fmt, Args&&... args)
        {
            label(fmt::format(fmt, std::forward<Args>(args)...).c_str(), flags);
        }

        template<typename... Args>
        void labelf_colored(const nk_flags align,
                            const xray::rendering::rgb_color& color,
                            const char* fmt,
                            Args&&... args)
        {
            label_colored(fmt::format(fmt, std::forward<Args>(args)...).c_str(), align, nk_rgba_fv(color.components));
        }

        template<typename... Args>
        void labelf_wrap(const char* fmt, Args&&... args)
        {
            label_wrap(fmt::format(fmt, std::forward<Args>(args)...).c_str());
        }

        template<typename... Args>
        void labelf_colored_wrap(const xray::rendering::rgb_color& color, const char* fmt, Args&&... args)
        {
            label_colored_wrap(fmt::format(fmt, std::forward<Args>(args)...).c_str(), color);
        }
    } text{ this };

    struct _button
    {
      private:
        friend class user_interface;
        user_interface* _owner;
        _button(user_interface* owner)
            : _owner{ owner }
        {
        }

      public:
        int32_t text(const char* title) { return nk_button_text(_owner->ctx(), title, strlen(title)); }

        int32_t label(const char* title) { return nk_button_label(_owner->ctx(), title); }

        int32_t color(const xray::rendering::rgb_color& color)
        {
            return nk_button_color(_owner->ctx(), nk_rgba_fv(color.components));
        }

        int32_t symbol(const enum nk_symbol_type stype) { return nk_button_symbol(_owner->ctx(), stype); }

        int32_t image(struct nk_image img) { return nk_button_image(_owner->ctx(), img); }

        int32_t symbol_label(const enum nk_symbol_type stype, const char* title, const nk_flags text_alignment)
        {
            return nk_button_symbol_label(_owner->ctx(), stype, title, text_alignment);
        }

        int32_t symbol_text(const enum nk_symbol_type stype, const char* title, const nk_flags alignment)
        {
            return nk_button_symbol_text(_owner->ctx(), stype, title, strlen(title), alignment);
        }

        int32_t image_label(struct nk_image img, const char* title, const nk_flags text_alignment)
        {
            return nk_button_image_label(_owner->ctx(), img, title, text_alignment);
        }

        int32_t image_text(struct nk_image img, const char* title, const nk_flags alignment)
        {
            return nk_button_image_text(_owner->ctx(), img, title, strlen(title), alignment);
        }

        int32_t text_styled(const struct nk_style_button* style, const char* title)
        {
            return nk_button_text_styled(_owner->ctx(), style, title, strlen(title));
        }

        int32_t label_styled(const struct nk_style_button* style, const char* title)
        {
            return nk_button_label_styled(_owner->ctx(), style, title);
        }

        int32_t symbol_styled(const struct nk_style_button* style, const enum nk_symbol_type stype)
        {
            return nk_button_symbol_styled(_owner->ctx(), style, stype);
        }

        int32_t image_styled(const struct nk_style_button* style, struct nk_image img)
        {
            return nk_button_image_styled(_owner->ctx(), style, img);
        }

        int32_t symbol_text_styled(const struct nk_style_button* style,
                                   const enum nk_symbol_type stype,
                                   const char* text,
                                   const nk_flags alignment)
        {
            return nk_button_symbol_text_styled(_owner->ctx(), style, stype, text, strlen(text), alignment);
        }

        int32_t symbol_label_styled(const struct nk_style_button* style,
                                    const enum nk_symbol_type symbol,
                                    const char* title,
                                    const nk_flags align)
        {
            return nk_button_symbol_label_styled(_owner->ctx(), style, symbol, title, align);
        }

        int32_t image_label_styled(const struct nk_style_button* style,
                                   struct nk_image img,
                                   const char* text,
                                   const nk_flags text_alignment)
        {
            return nk_button_image_label_styled(_owner->ctx(), style, img, text, text_alignment);
        }

        int32_t image_text_styled(const struct nk_style_button* style,
                                  struct nk_image img,
                                  const char* text,
                                  const nk_flags alignment)
        {
            return nk_button_image_text_styled(_owner->ctx(), style, img, text, strlen(text), alignment);
        }

        void set_behavior(const enum nk_button_behavior behavior)
        {
            return nk_button_set_behavior(_owner->ctx(), behavior);
        }

        int32_t push_behavior(const enum nk_button_behavior behavior)
        {
            return nk_button_push_behavior(_owner->ctx(), behavior);
        }

        int32_t pop_behavior() { return nk_button_pop_behavior(_owner->ctx()); }

    } button{ this };

    struct _checkbox
    {
      private:
        friend class user_interface;
        user_interface* _owner;
        _checkbox(user_interface* owner)
            : _owner{ owner }
        {
        }

      public:
        int32_t label(const char* text, const int32_t active) { return nk_check_label(_owner->ctx(), text, active); }

        int32_t text(const char* text, const int32_t active)
        {
            return nk_check_text(_owner->ctx(), text, strlen(text), active);
        }

        uint32_t flags_label(const char* text, const uint32_t flags, const uint32_t value)
        {
            return nk_check_flags_label(_owner->ctx(), text, flags, value);
        }

        uint32_t flags_text(const char* text, const uint32_t flags, const uint32_t value)
        {
            return nk_check_flags_text(_owner->ctx(), text, strlen(text), flags, value);
        }

        int32_t label(const char* text, int32_t* active) { return nk_checkbox_label(_owner->ctx(), text, active); }

        int32_t text(const char* text, int32_t* active)
        {
            return nk_checkbox_text(_owner->ctx(), text, strlen(text), active);
        }

        int32_t flags_label(const char* text, uint32_t* flags, const uint32_t value)
        {
            return nk_checkbox_flags_label(_owner->ctx(), text, flags, value);
        }

        int32_t flags_text(const char* text, uint32_t* flags, uint32_t value)
        {
            return nk_checkbox_flags_text(_owner->ctx(), text, strlen(text), flags, value);
        }
    } checkbox{ this };

    struct _radio
    {
      private:
        friend class user_interface;
        user_interface* _owner;
        _radio(user_interface* owner)
            : _owner{ owner }
        {
        }

      public:
        int32_t label(const char* text, int32_t* active) { return nk_radio_label(_owner->ctx(), text, active); }

        int32_t text(const char* text, int32_t* active)
        {
            return nk_radio_text(_owner->ctx(), text, strlen(text), active);
        }

        int32_t option_label(const char* text, const int32_t active)
        {
            return nk_option_label(_owner->ctx(), text, active);
        }

        int32_t option_text(const char* text, const int32_t active)
        {
            return nk_option_text(_owner->ctx(), text, strlen(text), active);
        }
    } radio{ this };

    struct _selectable
    {
      private:
        friend class user_interface;
        user_interface* _owner;
        _selectable(user_interface* owner)
            : _owner{ owner }
        {
        }

        int32_t label(const char* text, const nk_flags align, int32_t* value)
        {
            return nk_selectable_label(_owner->ctx(), text, align, value);
        }

        int32_t text(const char* text, const nk_flags align, int32_t* value)
        {
            return nk_selectable_text(_owner->ctx(), text, strlen(text), align, value);
        }

        int32_t image_label(struct nk_image img, const char* text, const nk_flags align, int32_t* value)
        {
            return nk_selectable_image_label(_owner->ctx(), img, text, align, value);
        }

        int32_t image_text(struct nk_image img, const char* text, const nk_flags align, int32_t* value)
        {
            return nk_selectable_image_text(_owner->ctx(), img, text, strlen(text), align, value);
        }

        int32_t label(const char* text, const nk_flags align, const int32_t value)
        {
            return nk_select_label(_owner->ctx(), text, align, value);
        }

        int32_t text(const char* text, const nk_flags align, int32_t value)
        {
            return nk_select_text(_owner->ctx(), text, strlen(text), align, value);
        }

        int32_t image_label(struct nk_image img, const char* text, const nk_flags align, const int32_t value)
        {
            return nk_select_image_label(_owner->ctx(), img, text, align, value);
        }

        int32_t image_text(struct nk_image img, const char* text, const nk_flags align, const int32_t value)
        {
            return nk_select_image_text(_owner->ctx(), img, text, strlen(text), align, value);
        }
    } selectable{ this };

    struct _slider
    {
      private:
        friend class user_interface;
        user_interface* _owner;
        _slider(user_interface* owner)
            : _owner{ owner }
        {
        }

      public:
        float float_(const float min, const float val, const float max, const float step)
        {
            return nk_slide_float(_owner->ctx(), min, val, max, step);
        }

        int32_t int_(const int32_t min, const int32_t val, const int32_t max, const int32_t step)
        {
            return nk_slide_int(_owner->ctx(), min, val, max, step);
        }

        int32_t float_(const float min, float* val, const float max, const float step)
        {
            return nk_slider_float(_owner->ctx(), min, val, max, step);
        }

        int32_t int_(const int32_t min, int32_t* val, const int32_t max, const int32_t step)
        {
            return nk_slider_int(_owner->ctx(), min, val, max, step);
        }
    } slider{ this };

    struct _progress
    {
      private:
        friend class user_interface;
        user_interface* _owner;
        _progress(user_interface* owner)
            : _owner{ owner }
        {
        }

      public:
        int32_t bar(nk_size* cur, const nk_size max, const int32_t modifyable)
        {
            return nk_progress(_owner->ctx(), cur, max, modifyable);
        }

        nk_size bar(const nk_size cur, const nk_size max, const int32_t modifyable)
        {
            return nk_prog(_owner->ctx(), cur, max, modifyable);
        }

    } progress{ this };

    struct _colorpicker
    {
      private:
        friend class user_interface;
        user_interface* _owner;
        _colorpicker(user_interface* owner)
            : _owner{ owner }
        {
        }

      public:
        xray::rendering::rgb_color picker(const xray::rendering::rgb_color& color)
        {
            xray::rendering::rgb_color picked_clr;
            nk_color_fv(picked_clr.components, nk_color_picker(_owner->ctx(), nk_rgba_fv(color.components), NK_RGBA));

            return picked_clr;
        }

        int32_t pick(xray::rendering::rgb_color* clr)
        {
            nk_color c{ nk_rgba_fv(clr->components) };
            const auto result = nk_color_pick(_owner->ctx(), &c, NK_RGBA);
            if (result) {
                nk_color_fv(clr->components, c);
            }

            return result;
        }

    } color{ this };

    struct _property
    {
      private:
        friend class user_interface;
        user_interface* _owner;
        _property(user_interface* owner)
            : _owner{ owner }
        {
        }

      public:
        void int_(const char* name,
                  const int32_t min,
                  int32_t* val,
                  const int32_t max,
                  const int32_t step,
                  const float inc_per_pixel)
        {
            nk_property_int(_owner->ctx(), name, min, val, max, step, inc_per_pixel);
        }

        void float_(const char* name,
                    const float min,
                    float* val,
                    const float max,
                    const float step,
                    const float inc_per_pixel)
        {
            nk_property_float(_owner->ctx(), name, min, val, max, step, inc_per_pixel);
        }

        void double_(const char* name,
                     const double min,
                     double* val,
                     const double max,
                     const double step,
                     const float inc_per_pixel)
        {
            return nk_property_double(_owner->ctx(), name, min, val, max, step, inc_per_pixel);
        }

        int32_t int_(const char* name,
                     const int32_t min,
                     const int32_t val,
                     const int32_t max,
                     const int32_t step,
                     const float inc_per_pixel)
        {
            return nk_propertyi(_owner->ctx(), name, min, val, max, step, inc_per_pixel);
        }

        float float_(const char* name,
                     const float min,
                     const float val,
                     const float max,
                     const float step,
                     const float inc_per_pixel)
        {
            return nk_propertyf(_owner->ctx(), name, min, val, max, step, inc_per_pixel);
        }

        double double_(const char* name,
                       const double min,
                       const double val,
                       const double max,
                       const double step,
                       const float inc_per_pixel)
        {
            return nk_propertyd(_owner->ctx(), name, min, val, max, step, inc_per_pixel);
        }

    } property{ this };

    struct _popup
    {
      private:
        friend class user_interface;
        user_interface* _owner;
        _popup(user_interface* owner)
            : _owner{ owner }
        {
        }

      public:
        int32_t begin(const enum nk_popup_type ptype,
                      const char* title,
                      const nk_flags flags,
                      const float x,
                      const float y,
                      const float w,
                      const float h)
        {
            return nk_popup_begin(_owner->ctx(), ptype, title, flags, nk_rect(x, y, w, h));
        }

        void close() { nk_popup_close(_owner->ctx()); }

        void end() { nk_popup_end(_owner->ctx()); }

    } popup{ this };

    struct _widget;
    friend struct _widget;

    struct _widget
    {
      private:
        friend class user_interface;
        user_interface* _owner;
        _widget(user_interface* owner)
            : _owner{ owner }
        {
        }

      public:
        xray::math::vec4f bounds()
        {
            const auto ret = nk_widget_bounds(_owner->ctx());
            return { ret.x, ret.y, ret.w, ret.h };
        }

        xray::math::vec2f position()
        {
            const auto res = nk_widget_position(_owner->ctx());
            return { res.x, res.y };
        }

        xray::math::vec2f size()
        {
            const auto res = nk_widget_size(_owner->ctx());
            return { res.x, res.y };
        }

        float width() { return nk_widget_width(_owner->ctx()); }

        float height() { return nk_widget_height(_owner->ctx()); }

        int32_t is_hovered() { return nk_widget_is_hovered(_owner->ctx()); }

        int32_t is_mouse_clicked(const xray::ui::mouse_button btn)
        {
            return nk_widget_is_mouse_clicked(_owner->ctx(), user_interface::translate_mouse_button(btn));
        }

        int32_t has_mouse_click_down(const xray::ui::mouse_button btn, const int32_t down)
        {
            return nk_widget_has_mouse_click_down(_owner->ctx(), user_interface::translate_mouse_button(btn), down);
        }

        void spacing(const int32_t cols) { return nk_spacing(_owner->ctx(), cols); }

    } widget{ this };

    struct _style;
    friend struct _style;

    struct _style
    {
      private:
        friend class user_interface;
        user_interface* _owner;
        _style(user_interface* owner)
            : _owner{ owner }
        {
        }

      public:
        void default_() { nk_style_default(_owner->ctx()); }

        void set_font(const char* font_name)
        {
            auto fnt = _owner->find_font(font_name);
            if (fnt) {
                nk_style_set_font(_owner->ctx(), &fnt->handle);
            }
        }

        int32_t set_cursor(const enum nk_style_cursor cursor) { return nk_style_set_cursor(_owner->ctx(), cursor); }

        void show_cursor() { nk_style_show_cursor(_owner->ctx()); }

        void hide_cursor() { nk_style_hide_cursor(_owner->ctx()); }

        int32_t push_font(const char* name)
        {
            auto fnt = _owner->find_font(name);
            if (fnt) {
                return nk_style_push_font(_owner->ctx(), &fnt->handle);
            }
            return 0;
        }

        int32_t push_float(float* old_val, const float newval)
        {
            return nk_style_push_float(_owner->ctx(), old_val, newval);
        }

        int32_t push_vec2(xray::math::vec2f* old, const xray::math::vec2f new_val)
        {
            return nk_style_push_vec2(
                _owner->ctx(), reinterpret_cast<struct nk_vec2*>(old), nk_vec2v(new_val.components));
        }

        int32_t push_style_item(struct nk_style_item* old, struct nk_style_item new_item)
        {
            return nk_style_push_style_item(_owner->ctx(), old, new_item);
        }

        int32_t push_flags(nk_flags* old, const nk_flags new_flags)
        {
            return nk_style_push_flags(_owner->ctx(), old, new_flags);
        }

        int32_t push_color(xray::rendering::rgb_color* old, const xray::rendering::rgb_color& new_color)
        {
            nk_color pcolor;
            const auto result = nk_style_push_color(_owner->ctx(), &pcolor, nk_rgba_fv(new_color.components));

            if (result) {
                nk_color_fv(old->components, pcolor);
            }

            return result;
        }

        int32_t pop_font() { return nk_style_pop_font(_owner->ctx()); }

        int32_t pop_float() { return nk_style_pop_float(_owner->ctx()); }

        int32_t pop_vec2() { return nk_style_pop_vec2(_owner->ctx()); }

        int32_t pop_style_item() { return nk_style_pop_style_item(_owner->ctx()); }

        int32_t pop_flags() { return nk_style_pop_flags(_owner->ctx()); }

        int32_t pop_color() { return nk_style_pop_color(_owner->ctx()); }

    } style{ this };

    struct _combobox
    {
      private:
        friend class user_interface;
        user_interface* _owner;
        _combobox(user_interface* owner)
            : _owner{ owner }
        {
        }

      public:
        int32_t combo(const char** items,
                      size_t count,
                      int32_t selected,
                      int32_t item_height,
                      const xray::math::vec2f& size)
        {
            return nk_combo(
                _owner->ctx(), items, static_cast<int32_t>(count), selected, item_height, nk_vec2v(size.components));
        }

        int32_t combo_separator(const char* items_separated_by_separator,
                                int32_t separator,
                                int32_t selected,
                                size_t count,
                                int32_t item_height,
                                const xray::math::vec2f& size)
        {
            return nk_combo_separator(_owner->ctx(),
                                      items_separated_by_separator,
                                      separator,
                                      selected,
                                      static_cast<int32_t>(count),
                                      item_height,
                                      nk_vec2v(size.components));
        }

        int32_t combo_string(const char* items_separated_by_zeros,
                             int32_t selected,
                             size_t count,
                             int item_height,
                             const xray::math::vec2f& size)
        {
            return nk_combo_string(_owner->ctx(),
                                   items_separated_by_zeros,
                                   selected,
                                   static_cast<int32_t>(count),
                                   item_height,
                                   nk_vec2v(size.components));
        }

        int32_t combo_callback(void (*item_getter)(void*, int32_t, const char**),
                               void* userdata,
                               int32_t selected,
                               size_t count,
                               int32_t item_height,
                               const xray::math::vec2f& size)
        {
            return nk_combo_callback(_owner->ctx(),
                                     item_getter,
                                     userdata,
                                     selected,
                                     static_cast<int32_t>(count),
                                     item_height,
                                     nk_vec2v(size.components));
        }

        void combobox(const char** items,
                      size_t count,
                      int32_t* selected,
                      int32_t item_height,
                      const xray::math::vec2f& size)
        {
            return nk_combobox(
                _owner->ctx(), items, static_cast<int32_t>(count), selected, item_height, nk_vec2v(size.components));
        }

        void combobox_string(const char* items_separated_by_zeros,
                             int32_t* selected,
                             size_t count,
                             int32_t item_height,
                             const xray::math::vec2f& size)
        {
            return nk_combobox_string(_owner->ctx(),
                                      items_separated_by_zeros,
                                      selected,
                                      static_cast<int32_t>(count),
                                      item_height,
                                      nk_vec2v(size.components));
        }

        void combobox_separator(struct nk_context*,
                                const char* items_separated_by_separator,
                                int32_t separator,
                                int32_t* selected,
                                size_t count,
                                int32_t item_height,
                                const xray::math::vec2f& size)
        {
            return nk_combobox_separator(_owner->ctx(),
                                         items_separated_by_separator,
                                         separator,
                                         selected,
                                         static_cast<int32_t>(count),
                                         item_height,
                                         nk_vec2v(size.components));
        }

        void combobox_callback(void (*item_getter)(void*, int32_t, const char**),
                               void* userdata,
                               int32_t* selected,
                               int32_t count,
                               int32_t item_height,
                               const xray::math::vec2f& size)
        {
            return nk_combobox_callback(_owner->ctx(),
                                        item_getter,
                                        userdata,
                                        selected,
                                        static_cast<int32_t>(count),
                                        item_height,
                                        nk_vec2v(size.components));
        }
    } combo{ this };

    struct _abstract_combo
    {
      private:
        friend class user_interface;
        _abstract_combo(user_interface* owner)
            : _owner{ owner }
        {
        }

      public:
        int begin_text(const char* selected, const xray::math::vec2f& size)
        {
            return nk_combo_begin_text(_owner->ctx(), selected, strlen(selected), nk_vec2v(size.components));
        }

        int begin_label(const char* selected, const xray::math::vec2f& size)
        {
            return nk_combo_begin_label(_owner->ctx(), selected, nk_vec2v(size.components));
        }

        int begin_color(const xray::rendering::rgb_color& clr, const xray::math::vec2f size)
        {
            return nk_combo_begin_color(_owner->ctx(), nk_rgba_fv(clr.components), nk_vec2v(size.components));
        }

        int begin_symbol(enum nk_symbol_type stype, const xray::math::vec2f& size)
        {
            return nk_combo_begin_symbol(_owner->ctx(), stype, nk_vec2v(size.components));
        }

        int begin_symbol_label(const char* selected, enum nk_symbol_type stype, const xray::math::vec2f& size)
        {
            return nk_combo_begin_symbol_label(_owner->ctx(), selected, stype, nk_vec2v(size.components));
        }

        int begin_symbol_text(const char* selected, enum nk_symbol_type stype, const xray::math::vec2f& size)
        {
            return nk_combo_begin_symbol_text(
                _owner->ctx(), selected, strlen(selected), stype, nk_vec2v(size.components));
        }

        int begin_image(struct nk_image img, const xray::math::vec2f& size)
        {
            return nk_combo_begin_image(_owner->ctx(), img, nk_vec2v(size.components));
        }

        int begin_image_label(const char* selected, struct nk_image img, const xray::math::vec2f& size)
        {
            return nk_combo_begin_image_label(_owner->ctx(), selected, img, nk_vec2v(size.components));
        }

        int begin_image_text(const char* selected, struct nk_image img, const xray::math::vec2f& size)
        {
            return nk_combo_begin_image_text(_owner->ctx(), selected, strlen(selected), img, nk_vec2v(size.components));
        }

        int item_label(const char* txt, nk_flags alignment)
        {
            return nk_combo_item_label(_owner->ctx(), txt, alignment);
        }

        int item_text(const char* txt, nk_flags alignment)
        {
            return nk_combo_item_text(_owner->ctx(), txt, strlen(txt), alignment);
        }

        int item_image_label(struct nk_image img, const char* txt, nk_flags alignment)
        {
            return nk_combo_item_image_label(_owner->ctx(), img, txt, alignment);
        }

        int item_image_text(struct nk_image img, const char* txt, nk_flags alignment)
        {
            return nk_combo_item_image_text(_owner->ctx(), img, txt, strlen(txt), alignment);
        }

        int item_symbol_label(enum nk_symbol_type stype, const char* txt, nk_flags alignment)
        {
            return nk_combo_item_symbol_label(_owner->ctx(), stype, txt, alignment);
        }

        int item_symbol_text(enum nk_symbol_type stype, const char* txt, nk_flags alignment)
        {
            return nk_combo_item_symbol_text(_owner->ctx(), stype, txt, strlen(txt), alignment);
        }

        void combo_close() { nk_combo_close(_owner->ctx()); }

        void end() { nk_combo_end(_owner->ctx()); }

      private:
        user_interface* _owner;
    } abstract_combo{ this };

    float widget_width() noexcept { return nk_widget_width(ctx()); }

  private:
    void init(const font_load_info* fonts_to_load, const size_t num_fonts);

    void upload_font_atlas(const void* pixels, const xray::math::vec2i32& size);

    void setup_render_data();

    static constexpr auto double_click_treshold_ms_low = 100u;
    static constexpr auto double_click_treshold_ms_high = 200u;
    static constexpr auto max_vertex_buffer_bytes = 1024u * 1024u;
    static constexpr auto max_index_buffer_bytes = 512u * 1024u;

    struct render_state
    {
        nk_buffer cmds;
        nk_draw_null_texture null;
        nk_font* font;
        std::unordered_map<std::string, nk_font*> fonts;
        nk_font_atlas font_atlas;
        nk_context ctx;
        xray::rendering::scoped_buffer vb;
        xray::rendering::scoped_buffer ib;
        xray::rendering::scoped_vertex_array vao;
        xray::rendering::vertex_program vs;
        xray::rendering::fragment_program fs;
        xray::rendering::program_pipeline pp;
        xray::rendering::scoped_texture font_tex;
        xray::rendering::scoped_sampler sampler;
        std::chrono::high_resolution_clock::time_point last_left_click;
        nk_anti_aliasing aa{ NK_ANTI_ALIASING_ON };
    } _renderer;

    XRAY_NO_COPY(user_interface);
}; // namespace ui

/// @}

} // namespace ui
} // namespace xray
