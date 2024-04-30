#include "renderer_dx10.hpp"
#include "xray/base/array_dimension.hpp"
#include "xray/base/debug/debug_ext.hpp"

using namespace xray::base;

xray::rendering::directx10::renderer::renderer(const render_params_t& r_param) noexcept
{

    {
        DXGI_SWAP_CHAIN_DESC chain_desc;
        chain_desc.BufferDesc.Width = r_param.width;
        chain_desc.BufferDesc.Height = r_param.height;
        chain_desc.BufferDesc.RefreshRate.Numerator = 60;
        chain_desc.BufferDesc.RefreshRate.Denominator = 1;
        chain_desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        chain_desc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
        chain_desc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
        chain_desc.SampleDesc.Count = 1;
        chain_desc.SampleDesc.Quality = 0;
        chain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        chain_desc.BufferCount = 1;
        chain_desc.OutputWindow = r_param.output_wnd;
        chain_desc.Windowed = true;
        chain_desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
        chain_desc.Flags = 0;

        constexpr auto kDriverType = D3D10_DRIVER_TYPE_HARDWARE;
        constexpr auto kCreateFlags = D3D10_CREATE_DEVICE_SINGLETHREADED | D3D10_CREATE_DEVICE_DEBUG;

        D3D10CreateDeviceAndSwapChain(nullptr,
                                      kDriverType,
                                      nullptr,
                                      kCreateFlags,
                                      D3D10_SDK_VERSION,
                                      &chain_desc,
                                      raw_ptr_ptr(swapchain_),
                                      raw_ptr_ptr(device_));

        if (!swapchain_ || !device_) {
            OUTPUT_DBG_MSG("Failed to create device/swapchain !!!");
            return;
        }
    }

    {
        com_ptr<ID3D10Texture2D> backbuffer_tex;
        swapchain_->GetBuffer(0, __uuidof(ID3D10Texture2D), reinterpret_cast<void**>(raw_ptr_ptr(backbuffer_tex)));

        assert(backbuffer_tex != nullptr);
        device_->CreateRenderTargetView(raw_ptr(backbuffer_tex), nullptr, raw_ptr_ptr(rtv_));
        if (!rtv_) {
            OUTPUT_DBG_MSG("Failed to create render target view!");
            return;
        }

        ID3D10RenderTargetView* const rtvs[] = { raw_ptr(rtv_) };
        device_->OMSetRenderTargets(1, rtvs, nullptr);
    }

    {
        const auto view_port = D3D10_VIEWPORT{ 0, 0, r_param.width, r_param.height, 0.0f, 1.0f };

        device_->RSSetViewports(1, &view_port);
    }
}

xray::rendering::directx10::renderer::~renderer()
{
    if (swapchain_) {
        BOOL fs_state{ false };
        IDXGIOutput* output{ nullptr };
        swapchain_->GetFullscreenState(&fs_state, &output);
        if (fs_state)
            swapchain_->SetFullscreenState(false, nullptr);
    }
}

void
xray::rendering::directx10::renderer::clear_backbuffer(const float red,
                                                       const float green,
                                                       const float blue,
                                                       const float alpha /* = 1.0f */) noexcept
{
    assert(valid());
    const float clr_color[] = { red, green, blue, alpha };
    device_->ClearRenderTargetView(raw_ptr(rtv_), clr_color);
}

void
xray::rendering::directx10::renderer::swap_buffers() noexcept
{
    assert(valid());
    swapchain_->Present(0, 0);
}
