#pragma once

#include "xray/xray.hpp"

namespace xray {
namespace rendering {

#if defined(XRAY_RENDERER_DIRECTX)
inline namespace directx10 {
#else
namespace directx10 {
#endif

enum class shader_type : uint8_t { vertex, geometry, fragment };

template <shader_type st>
struct shader_traits;

} // namespace directx10
} // namespace rendering
} // namespace xray
