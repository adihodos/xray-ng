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

#include "xray/base/unique_pointer.hpp"
#include "xray/ui/user_interface_render_context.hpp"
#include "xray/xray.hpp"

#if defined(XRAY_RENDERER_DIRECTX)
#include "xray/base/windows/com_ptr.hpp"
#include "xray/rendering/directx/pixel_shader.hpp"
#include "xray/rendering/directx/vertex_shader.hpp"
#include <d3d11.h>
#else
#endif

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>
#include <span>

#include <concurrencpp/forward_declarations.h>

#include <imgui/IconsFontAwesome.h>
#include <imgui/imconfig.h>
#include <imgui/imgui.h>

#include <tl/optional.hpp>
#include <mio/mmap.hpp>

struct ImGuiIO;
struct ImFont;

namespace xray {
namespace ui {

/// \addtogroup __GroupXrayUI
/// @{

struct window_event;

struct font_info
{
    std::filesystem::path path;
    float pixel_size;
};

struct imcontext_deleter
{
    void operator()(ImGuiContext* p) noexcept
    {
        if (p) {
            ImGui::DestroyContext(p);
        }
    }
};

struct UserInterfaceBackendCreateInfo;

struct FontsLoadBundle
{
    std::vector<font_info> info;
    std::vector<mio::mmap_source> data;
};

class user_interface
{
  public:
    static constexpr uint32_t VERTEX_BUFFER_SIZE = 1024 * 512;
    static constexpr uint32_t INDEX_BUFFER_SIZE = 1024 * 128;

    user_interface() noexcept;
    user_interface(const std::span<const font_info> font_list);
    explicit user_interface(concurrencpp::result<FontsLoadBundle> font_pkg_future);
    ~user_interface() noexcept;

    bool input_event(const window_event& evt);
    bool wants_input() const noexcept;

    void new_frame(const int32_t wnd_width, const int32_t wnd_height);
    void tick(const float delta);

    tl::optional<UserInterfaceRenderContext> draw();

    void set_global_font(const char* name);
    void push_font(const char* name);
    void pop_font();

    void set_current() { ImGui::SetCurrentContext(xray::base::raw_ptr(_imcontext)); }
    static void font_atlas_upload_callback(const uint32_t atlas_id, void* context) noexcept
    {
        (static_cast<user_interface*>(context))->font_atlas_upload_done(atlas_id);
    }
    UserInterfaceBackendCreateInfo render_backend_create_info() noexcept;

  private:
    struct loaded_font
    {
        loaded_font() = default;
        loaded_font(std::string n, const float size, ImFont* f)
            : name{ std::move(n) }
            , pixel_size{ size }
            , font{ f }
        {
        }

        std::string name;
        float pixel_size;
        ImFont* font;
    };

    loaded_font* find_font(const char* name = nullptr);
    void init(const std::span<const font_info> font_list);
    void load_fonts(const std::span<const font_info> font_list);
    void font_atlas_upload_done(const uint32_t atlas_id) noexcept { _rendercontext.font_atlas_handle = atlas_id; }

    struct render_context
    {
        std::vector<loaded_font> fonts;
        uint32_t font_atlas_handle{};
        uint8_t* atlas_data{};
        int32_t atlas_width{};
        int32_t atlas_height{};
#if defined(XRAY_RENDERER_DIRECTX)
        ID3D11Device* device;
        ID3D11DeviceContext* context;
        xray::base::com_ptr<ID3D11Buffer> vertex_buffer;
        xray::base::com_ptr<ID3D11Buffer> index_buffer;
        xray::base::com_ptr<ID3D11InputLayout> input_layout;
        xray::rendering::vertex_shader vs;
        xray::rendering::pixel_shader ps;
        xray::base::com_ptr<ID3D11ShaderResourceView> font_texture;
        xray::base::com_ptr<ID3D11SamplerState> font_sampler;
        xray::base::com_ptr<ID3D11BlendState> blend_state;
        xray::base::com_ptr<ID3D11RasterizerState> raster_state;
        xray::base::com_ptr<ID3D11DepthStencilState> depth_stencil_state;
#else
#endif
    } _rendercontext;

    xray::base::unique_pointer<ImGuiContext, imcontext_deleter> _imcontext;

  private:
    XRAY_NO_COPY(user_interface);
};

/// @}

} // namespace ui
} // namespace xray
