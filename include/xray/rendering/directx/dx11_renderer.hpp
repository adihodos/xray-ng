//
// Copyright (c) 2011, 2012, 2013 Adrian Hodos
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

/// \file dx11_renderer.hpp

#include "xray/base/windows/com_ptr.hpp"
#include "xray/xray.hpp"
#include <cstdint>
#include <d3d11.h>
#include <dxgi.h>
#include <windows.h>

namespace xray {
namespace rendering {

/// \addtogroup __GroupXrayRendering_Directx
/// @{

/// \brief  Parameters for the renderer.
struct renderer_init_params
{

    DXGI_FORMAT render_target_format;

    DXGI_FORMAT depth_stencil_format;

    ///<    Use hardware accelerated rendering of WARP.
    D3D_DRIVER_TYPE driver_type;

    ///<  Options for creating the device.
    uint32_t create_device_opts;

    ///<    Output window for the rendered image.
    HWND output_window;
};

class dx11_renderer
{
  public:
    using device_handle_type = ID3D11Device*;
    using device_context_handle_type = ID3D11DeviceContext*;

  public:
    explicit dx11_renderer(const renderer_init_params& init_params);

    ~dx11_renderer();

    /// \name   Sanity checking.
    /// @{

  public:
    bool valid() const noexcept { return device_valid() && resources_valid(); }

    explicit operator bool() const noexcept { return valid(); }

    ID3D11Device* device() const noexcept
    {
        assert(valid());
        return xray::base::raw_ptr(dx_device_);
    }

    ID3D11DeviceContext* context() const noexcept
    {
        assert(valid());
        return xray::base::raw_ptr(devcontext_);
    }

    /// @}

  public:
    uint32_t render_target_width() const noexcept { return output_wnd_width_; }

    uint32_t render_target_height() const noexcept { return output_wnd_height_; }

    /// \name   Swap chain and presentation management.
    /// @{

  public:
    /// \brief  Present the rendered image to the output window.
    /// \param  test_for_occlusion  If true, the swap chain will simply test if
    ///         the output window is occluded and return a corresponding code.
    HRESULT swap_buffers(const bool test_for_occlusion = false) const noexcept;

    /// \brief  Toggle between fullscreen/windowed mode rendering.
    void toggle_fullscreen() noexcept;

    /// \brief  Called when the output window has been resized.
    /// \param  new_width   Width of the output window.
    /// \param  new_height  Height of the output window.
    void handle_resize(const uint32_t new_width, const uint32_t new_height) noexcept;

    void clear_render_target_view(const float red, const float green, const float blue, const float alpha = 1.0f);

    void clear_depth_stencil();

    /// @}

  private:
    void create_rtvs() noexcept;
    bool device_valid() const noexcept { return dx_device_ && devcontext_; }

    bool resources_valid() const noexcept
    {
        return depth_stencil_texture_ && render_target_view_ && depth_stencil_view_;
    }

  public:
    // struct d3d_data {
    //   ID3D11Device*        dev{nullptr};
    //   ID3D11DeviceContext* ctx{nullptr};
    // } D3D;

  private:
    xray::base::com_ptr<ID3D11Device> dx_device_;
    xray::base::com_ptr<ID3D11DeviceContext> devcontext_;
    xray::base::com_ptr<ID3D11RenderTargetView> render_target_view_;
    xray::base::com_ptr<ID3D11Texture2D> depth_stencil_texture_;
    xray::base::com_ptr<ID3D11DepthStencilView> depth_stencil_view_;
    xray::base::com_ptr<IDXGISwapChain> swap_chain_;
    uint32_t output_wnd_width_{ 0 };
    uint32_t output_wnd_height_{ 0 };
    DXGI_FORMAT backbuffer_format_;

    /// \name   Disabled operations
    /// @{

  private:
    dx11_renderer(dx11_renderer&&) = delete;
    dx11_renderer& operator=(dx11_renderer&&) = delete;
    dx11_renderer(const dx11_renderer&) = delete;
    dx11_renderer& operator=(const dx11_renderer&) = delete;

    /// @}
};

/// @}

} // namespace rendering
} // namespace xray
