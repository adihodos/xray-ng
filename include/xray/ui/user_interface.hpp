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
#include "xray/base/fast_delegate.hpp"

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

#include <cassert>
#include <cstdint>
#include <imgui/IconsFontAwesome.h>
#include <string>
#include <vector>

struct ImGuiIO;
struct ImFont;

namespace xray {
namespace ui {

/// \addtogroup __GroupXrayUI
/// @{

struct window_event;

struct font_info {
  std::string path;
  float       pixel_size;
};

class imgui_backend {
public:
  static constexpr uint32_t VERTEX_BUFFER_SIZE = 1024 * 512;
  static constexpr uint32_t INDEX_BUFFER_SIZE  = 1024 * 128;

#if defined(XRAY_RENDERER_DIRECTX)

  explicit imgui_backend(ID3D11Device*        device,
                         ID3D11DeviceContext* context) noexcept;
#else
  imgui_backend() noexcept;
#endif

  imgui_backend(const font_info* fonts, const size_t num_fonts);

  ~imgui_backend() noexcept;

  bool input_event(const window_event& evt);
  bool wants_input() const noexcept;

  void new_frame(const int32_t wnd_width, const int32_t wnd_height);
  void tick(const float delta);
  void draw();

  void set_global_font(const char* name);
  void push_font(const char* name);
  void pop_font();

private:
  struct loaded_font {
    std::string name;
    float       pixel_size;
    ImFont*     font;
  };

  loaded_font* find_font(const char* name = nullptr);
  void         init(const font_info* fonts, const size_t num_fonts);
  void         setup_key_mappings();

  struct render_context {
    std::vector<loaded_font> fonts;
#if defined(XRAY_RENDERER_DIRECTX)
    ID3D11Device*                                 device;
    ID3D11DeviceContext*                          context;
    xray::base::com_ptr<ID3D11Buffer>             vertex_buffer;
    xray::base::com_ptr<ID3D11Buffer>             index_buffer;
    xray::base::com_ptr<ID3D11InputLayout>        input_layout;
    xray::rendering::vertex_shader                vs;
    xray::rendering::pixel_shader                 ps;
    xray::base::com_ptr<ID3D11ShaderResourceView> font_texture;
    xray::base::com_ptr<ID3D11SamplerState>       font_sampler;
    xray::base::com_ptr<ID3D11BlendState>         blend_state;
    xray::base::com_ptr<ID3D11RasterizerState>    raster_state;
    xray::base::com_ptr<ID3D11DepthStencilState>  depth_stencil_state;
#else
    xray::rendering::scoped_buffer       _vertex_buffer;
    xray::rendering::scoped_buffer       _index_buffer;
    xray::rendering::scoped_vertex_array _vertex_arr;
    xray::rendering::vertex_program      _vs;
    xray::rendering::fragment_program    _fs;
    xray::rendering::program_pipeline    _pipeline;
    xray::rendering::scoped_texture      _font_texture;
    xray::rendering::scoped_sampler      _font_sampler;
#endif
    bool _valid{false};
  } _rendercontext;

  ImGuiIO* _gui;

  //  ImFont*  _small_font;
  //  ImFont*  _medium_font;

private:
  XRAY_NO_COPY(imgui_backend);
};

/// @}

} // namespace ui
} // namespace xray
