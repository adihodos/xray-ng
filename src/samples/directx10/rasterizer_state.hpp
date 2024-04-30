#pragma once

#include "xray/base/windows/com_ptr.hpp"
#include "xray/xray.hpp"
#include <d3d10.h>

namespace xray {
namespace rendering {

#if defined(XRAY_RENDERER_DIRECTX)
inline namespace directx10 {
#else
namespace directx10 {
#endif

struct rasterizer_state : public D3D10_RASTERIZER_DESC
{
    rasterizer_state() noexcept;
};

using scoped_rasterizer_state = base::com_ptr<ID3D10RasterizerState>;

} // namespace directx10
} // namespace rendering
} // namespace xray
