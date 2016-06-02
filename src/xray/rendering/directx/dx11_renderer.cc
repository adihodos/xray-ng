#include "xray/rendering/directx/dx11_renderer.hpp"
#include "xray/base/array_dimension.hpp"
#include "xray/base/logger.hpp"
#include "xray/base/maybe.hpp"
#include "xray/base/nothing.hpp"
#include "xray/base/pod_zero.hpp"
#include "xray/base/shims/string.hpp"
#include <cstdio>
#include <cstdlib>
#include <d3dcompiler.h>
#include <stlsoft/memory/auto_buffer.hpp>
#include <string>
#include <utility>

using namespace xray::base;
using namespace xray::rendering;

static const char* dxgi_error_code_to_string(const HRESULT dxgi_errcode) {
  switch (dxgi_errcode) {
  case DXGI_ERROR_DEVICE_HUNG:
    return "The application's device failed due to badly formed "
           "commands sent by the application. This is an design-time "
           "issue that should be investigated and fixed.";
    break;

  case DXGI_ERROR_DEVICE_REMOVED:
    return "The video card has been physically removed from the system, "
           "or a driver upgrade for the video card has occurred. "
           "The application should destroy and recreate the device.";
    break;

  case DXGI_ERROR_DEVICE_RESET:
    return "The device failed due to a badly formed command. "
           "This is a run - time issue; The application should destroy and "
           "recreate the device.";
    break;

  case DXGI_ERROR_DRIVER_INTERNAL_ERROR:
    return "The driver encountered a problem and was put into the "
           "device removed state.";
    break;

  case DXGI_ERROR_FRAME_STATISTICS_DISJOINT:
    return "An event(for example, a power cycle) interrupted the "
           "gathering of presentation statistics.";
    break;

  case DXGI_ERROR_GRAPHICS_VIDPN_SOURCE_IN_USE:
    return "The application attempted to acquire exclusive ownership of an "
           "output, but failed because some other application"
           "(or device within the application) already acquired ownership.";
    break;

  case DXGI_ERROR_INVALID_CALL:
    return "The application provided invalid parameter data; this must "
           "be debugged and fixed before the application is released";
    break;

  case DXGI_ERROR_MORE_DATA:
    return "The buffer supplied by the application is not big enough to "
           "hold the requested data.";
    break;

  case DXGI_ERROR_NONEXCLUSIVE:
    return "A global counter resource is in use, and the Direct3D device "
           "can't currently use the counter resource.";
    break;

  case DXGI_ERROR_NOT_CURRENTLY_AVAILABLE:
    return "The resource or request is not currently available, but it "
           "might become available later.";
    break;

  case DXGI_ERROR_NOT_FOUND:
    return "When calling IDXGIObject::GetPrivateData, the GUID passed in "
           "is not recognized as one previously passed to "
           "IDXGIObject::SetPrivateData or "
           "IDXGIObject::SetPrivateDataInterface."
           "When calling IDXGIFactory::EnumAdapters or "
           "IDXGIAdapter::EnumOutputs, "
           "the enumerated ordinal is out of range.";
    break;

  case DXGI_ERROR_REMOTE_CLIENT_DISCONNECTED:
  case DXGI_ERROR_REMOTE_OUTOFMEMORY:
    return "Reserved";
    break;

  case DXGI_ERROR_WAS_STILL_DRAWING:
    return "The GPU was busy at the moment when a call was made to "
           "perform an operation, and did not execute or schedule the "
           "operation.";
    break;

  case DXGI_ERROR_UNSUPPORTED:
    return "The requested functionality is not supported by the device "
           "or the driver.";
    break;

  case DXGI_ERROR_ACCESS_LOST:
    return "The desktop duplication interface is invalid.The desktop "
           "duplication interface typically becomes invalid when a "
           "different type of image is displayed on the desktop.";
    break;

  case DXGI_ERROR_WAIT_TIMEOUT:
    return "The time - out interval elapsed before the next desktop frame "
           "was available.";
    break;

  case DXGI_ERROR_SESSION_DISCONNECTED:
    return "The Remote Desktop Services session is currently disconnected";
    break;

  case DXGI_ERROR_RESTRICT_TO_OUTPUT_STALE:
    return "The DXGI output(monitor) to which the swap chain content was "
           "restricted is now disconnected or changed.";
    break;

  case DXGI_ERROR_CANNOT_PROTECT_CONTENT:
    return "DXGI can't provide content protection on the swap chain. "
           "This error is typically caused by an older driver, or when you "
           "use a swap chain that is incompatible with content protection.";
    break;

  case DXGI_ERROR_ACCESS_DENIED:
    return "You tried to use a resource to which you did not have the "
           "required access privileges.This error is most typically caused "
           "when you write to a shared resource with read - only access.";
    break;

  case DXGI_ERROR_NAME_ALREADY_EXISTS:
    return "The supplied name of a resource in a call to "
           "IDXGIResource1::CreateSharedHandle is already associated with "
           "some other resource.";
    break;

  default:
    break;
  }

  return "Unknown";
}

xray::rendering::dx11_renderer::dx11_renderer(
    const renderer_init_params& init_params) {

  //
  //  Init params.
  {
    RECT wnd_rc;
    GetClientRect(init_params.output_window, &wnd_rc);
    output_wnd_width_  = static_cast<uint32_t>(wnd_rc.right);
    output_wnd_height_ = static_cast<uint32_t>(wnd_rc.bottom);
    backbuffer_format_ = init_params.render_target_format;
  }

  //
  //  Create the DirectX device and device context objects.
  {

    D3D_FEATURE_LEVEL max_feat_lvl;
    const auto        ret_code = D3D11CreateDevice(
        nullptr, init_params.driver_type, nullptr,
        init_params.create_device_opts, nullptr, 0, D3D11_SDK_VERSION,
        raw_ptr_ptr(dx_device_), &max_feat_lvl, raw_ptr_ptr(devcontext_));

    if (FAILED(ret_code)) {
      XR_LOG_CRITICAL("Failed to create D3D device and context!");
      return;
    }

    //
    //  The swap chain must be created by the same IDXGIFactory object
    //  that created the DirectX device, so try to retrieve it.
    com_ptr<IDXGIFactory1> factory;
    {
      com_ptr<IDXGIDevice1> dxgi_device;
      {
        const auto result = dx_device_->QueryInterface(
            __uuidof(IDXGIDevice1),
            reinterpret_cast<void**>(raw_ptr_ptr(dxgi_device)));

        if (FAILED(result)) {
          XR_LOG_CRITICAL("Failed to obtain IDXGIDevice1 interface !");
          return;
        }
      }

      com_ptr<IDXGIAdapter1> dxgi_adapter;
      {
        const auto result = dxgi_device->GetParent(
            __uuidof(IDXGIAdapter1),
            reinterpret_cast<void**>(raw_ptr_ptr(dxgi_adapter)));

        if (FAILED(result)) {
          XR_LOG_CRITICAL("Failed to obtain IDXGIAdapter1 interface!");
          return;
        }
      }

      {
        const auto result = dxgi_adapter->GetParent(
            __uuidof(IDXGIFactory1),
            reinterpret_cast<void**>(raw_ptr_ptr(factory)));

        if (FAILED(result)) {
          XR_LOG_CRITICAL("Failed to obtain IDXGIFactory1 critical!");
          return;
        }
      }
    }

    maybe<DXGI_MODE_DESC> closest_mode_match{nothing{}};
    {

      //
      //  Use 0 for index so that we get the primary adapter.
      com_ptr<IDXGIAdapter1> primary_adapter;
      factory->EnumAdapters1(0, raw_ptr_ptr(primary_adapter));

      if (!primary_adapter) {
        return;
      }

      //
      //  Use 0 for index so that we get the primary output.
      com_ptr<IDXGIOutput> primary_output;
      primary_adapter->EnumOutputs(0, raw_ptr_ptr(primary_output));
      if (!primary_output)
        return;

      pod_zero<DXGI_MODE_DESC> desired_mode;
      desired_mode.Format = backbuffer_format_;
      desired_mode.Width  = output_wnd_width_;
      desired_mode.Height = output_wnd_height_;

      DXGI_MODE_DESC closest_match;

      if (primary_output->FindClosestMatchingMode(&desired_mode, &closest_match,
                                                  nullptr) == S_OK) {
        closest_mode_match = closest_match;
      }
    }

    if (!closest_mode_match) {
      XR_LOG_ERR("No display mode found with requested properties : {}x{}",
                 output_wnd_width_, output_wnd_height_);
      return;
    }

    const auto mode = closest_mode_match.value();

    XR_LOG_INFO("Renderer display resolution {} x {} @ {}/{}", mode.Width,
                mode.Height, mode.RefreshRate.Numerator,
                mode.RefreshRate.Denominator);

    //
    //  Create the swap chain.
    {
      pod_zero<DXGI_SWAP_CHAIN_DESC> swap_chain_desc;

      swap_chain_desc.BufferCount      = 1;
      swap_chain_desc.BufferDesc       = mode;
      swap_chain_desc.BufferUsage      = DXGI_USAGE_RENDER_TARGET_OUTPUT;
      swap_chain_desc.Flags            = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
      swap_chain_desc.OutputWindow     = init_params.output_window;
      swap_chain_desc.SampleDesc.Count = 1;
      swap_chain_desc.SwapEffect       = DXGI_SWAP_EFFECT_DISCARD;

      //
      //  Create swapchain as windowed first, if requested we'll set it to
      //  fullscreen later.
      swap_chain_desc.Windowed = true;

      const auto ret_code = factory->CreateSwapChain(
          raw_ptr(dx_device_), &swap_chain_desc, raw_ptr_ptr(swap_chain_));

      if (FAILED(ret_code)) {
        XR_LOG_CRITICAL("Failed to create swap chain, error {0}",
                        dxgi_error_code_to_string(ret_code));
        return;
      }
    }

    {
      //
      //  Inform DXGI we'll take care of handling fullscreen/windowed
      //  transitions.
      factory->MakeWindowAssociation(init_params.output_window,
                                     DXGI_MWA_NO_WINDOW_CHANGES |
                                         DXGI_MWA_NO_ALT_ENTER);

      create_rtvs();
    }
  }
}

xray::rendering::dx11_renderer::~dx11_renderer() {
  if (valid()) {
    devcontext_->ClearState();

    //
    //  Set swap chain to windowed mode before its destruction.
    com_ptr<IDXGIOutput> output_target;
    int32_t              is_fullscreen{false};
    swap_chain_->GetFullscreenState(&is_fullscreen, raw_ptr_ptr(output_target));

    if (is_fullscreen)
      swap_chain_->SetFullscreenState(false, nullptr);
  }
}

void xray::rendering::dx11_renderer::handle_resize(
    const uint32_t width, const uint32_t height) noexcept {
  if (!valid())
    return;

  {
    output_wnd_width_  = width;
    output_wnd_height_ = height;

    //
    //  Release any references to the backbuffers, otherwise
    //  ResizeBuffers() will fail.
    context()->OMSetRenderTargets(0, nullptr, nullptr);
    render_target_view_    = nullptr;
    depth_stencil_view_    = nullptr;
    depth_stencil_texture_ = nullptr;
  }

  //
  //  Now resize the buffers and recreate the render targets and depth
  //  stencil views.
  {
    const auto resize_result =
        swap_chain_->ResizeBuffers(1, 0, 0, backbuffer_format_, 0);

    if (FAILED(resize_result)) {
      XR_LOG_CRITICAL("Failed to resize buffers, error {0}",
                      dxgi_error_code_to_string(resize_result));
      return;
    }

    create_rtvs();
  }
}

void xray::rendering::dx11_renderer::clear_render_target_view(
    const float red, const float green, const float blue,
    const float alpha /*= 1.0f*/) {
  assert(valid());

  const float clear_color[] = {red, green, blue, alpha};
  devcontext_->ClearRenderTargetView(raw_ptr(render_target_view_), clear_color);
}

void xray::rendering::dx11_renderer::clear_depth_stencil() {
  assert(valid());
  devcontext_->ClearDepthStencilView(raw_ptr(depth_stencil_view_),
                                     D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL,
                                     1.0f, 0xFF);
}

void xray::rendering::dx11_renderer::create_rtvs() noexcept {
  assert(device_valid());

  //
  //  Stores information about the back buffer.
  D3D11_TEXTURE2D_DESC texture_desc;

  //
  //  Create the render target view of the backbuffer. This will be bound to
  //  the pipeline.
  {
    com_ptr<ID3D11Texture2D> backbuffer_tex;
    swap_chain_->GetBuffer(
        0, __uuidof(ID3D11Texture2D),
        reinterpret_cast<void**>(raw_ptr_ptr(backbuffer_tex)));

    if (!backbuffer_tex)
      return;

    backbuffer_tex->GetDesc(&texture_desc);

    const auto ret_code = dx_device_->CreateRenderTargetView(
        raw_ptr(backbuffer_tex), nullptr, raw_ptr_ptr(render_target_view_));

    if (FAILED(ret_code)) {
      XR_LOG_CRITICAL("Failed to create render target view for back surface!");
      return;
    }
  }

  //
  //  Create the depth stencil texture and the associated view.
  {
    D3D11_TEXTURE2D_DESC tex_stencil_desc;
    tex_stencil_desc.Width              = texture_desc.Width;
    tex_stencil_desc.Height             = texture_desc.Height;
    tex_stencil_desc.CPUAccessFlags     = 0;
    tex_stencil_desc.Format             = DXGI_FORMAT_D24_UNORM_S8_UINT;
    tex_stencil_desc.MiscFlags          = 0;
    tex_stencil_desc.Usage              = D3D11_USAGE_DEFAULT;
    tex_stencil_desc.SampleDesc.Count   = 1;
    tex_stencil_desc.SampleDesc.Quality = 0;
    tex_stencil_desc.ArraySize          = 1;
    tex_stencil_desc.BindFlags          = D3D11_BIND_DEPTH_STENCIL;
    tex_stencil_desc.MipLevels          = 0;

    {
      const auto ret_code = dx_device_->CreateTexture2D(
          &tex_stencil_desc, nullptr, raw_ptr_ptr(depth_stencil_texture_));

      if (FAILED(ret_code)) {
        XR_LOG_CRITICAL("Failed to create depth/stencil texture!");
        return;
      }
    }

    {
      const auto result_create_dsv = dx_device_->CreateDepthStencilView(
          raw_ptr(depth_stencil_texture_), nullptr,
          raw_ptr_ptr(depth_stencil_view_));

      if (FAILED(result_create_dsv)) {
        XR_LOG_CRITICAL("Failed to create depth/stecil view!");
        return;
      }
    }
  }

  //
  //  Bind the render target views and the depth stencil views to the
  //  output merger stage.
  {
    ID3D11RenderTargetView* const bound_render_targets[] = {
        raw_ptr(render_target_view_)};

    devcontext_->OMSetRenderTargets(XR_U32_COUNTOF__(bound_render_targets),
                                    bound_render_targets,
                                    raw_ptr(depth_stencil_view_));
  }

  //
  //  Update the viewports information.
  {
    D3D11_VIEWPORT viewport;
    viewport.TopLeftX = viewport.TopLeftY = 0.0f;
    viewport.Width    = static_cast<float>(texture_desc.Width);
    viewport.Height   = static_cast<float>(texture_desc.Height);
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;

    devcontext_->RSSetViewports(1, &viewport);
  }
}

HRESULT xray::rendering::dx11_renderer::swap_buffers(
    const bool test_for_occlusion /*= false*/) const noexcept {
  if (!valid())
    return E_FAIL;

  return swap_chain_->Present(0u, test_for_occlusion ? DXGI_PRESENT_TEST : 0u);
}

void xray::rendering::dx11_renderer::toggle_fullscreen() noexcept {
  if (!valid())
    return;

  com_ptr<IDXGIOutput> output_target;
  int32_t              fs_state{false};

  swap_chain_->GetFullscreenState(&fs_state, raw_ptr_ptr(output_target));
  swap_chain_->SetFullscreenState(!fs_state, nullptr);
}
