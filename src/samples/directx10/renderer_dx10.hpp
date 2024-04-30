#pragma once

#include "xray/base/unique_pointer.hpp"
#include "xray/base/windows/com_ptr.hpp"
#include "xray/xray.hpp"
#include <cassert>
#include <cstdint>
#include <d3d10.h>
#include <dxgi.h>

namespace xray {
namespace rendering {

#if defined(XRAY_RENDERER_DIRECTX)
inline namespace directx10 {
#else
namespace directx10 {
#endif

struct render_params_t
{
    uint32_t width;
    uint32_t height;
    HWND output_wnd;
    bool fullscreen;
};

class renderer
{
  public:
    using device_context_handle = ID3D10Device*;

    renderer(const render_params_t& r_param) noexcept;
    ~renderer();

    bool valid() const noexcept { return swapchain_ && device_ && rtv_; }

    explicit operator bool() const noexcept { return valid(); }

    device_context_handle dev_ctx() const noexcept
    {
        assert(valid());
        return base::raw_ptr(device_);
    }

    void clear_backbuffer(const float red, const float green, const float blue, const float alpha = 1.0f) noexcept;

    void swap_buffers() noexcept;

  private:
    base::com_ptr<IDXGISwapChain> swapchain_;
    base::com_ptr<ID3D10Device> device_;
    base::com_ptr<ID3D10RenderTargetView> rtv_;
    XRAY_NO_COPY(renderer);
};

} // namespace directx10
} // namespace rendering
} // namespace xray
