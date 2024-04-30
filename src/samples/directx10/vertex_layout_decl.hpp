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

using scoped_vertex_layout_decl = base::com_ptr<ID3D10InputLayout>;

} // namespace directx10
} // namespace rendering
} // namespace xray
