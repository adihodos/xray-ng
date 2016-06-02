#pragma once

#include "xray/xray.hpp"
#include <cassert>
#include <d3d10.h>

namespace xray {
namespace rendering {

#if defined(XRAY_RENDERER_DIRECTX)
inline namespace directx10 {
#else
namespace directx10 {
#endif

class scoped_buffer_mapping {
public:
  scoped_buffer_mapping(ID3D10Buffer* buffer, const D3D10_MAP type);
  ~scoped_buffer_mapping();

  bool valid() const noexcept { return mapping_ != nullptr; }
  explicit operator bool() const noexcept { return valid(); }

  void* memory() const noexcept {
    assert(valid());
    return mapping_;
  }

private:
  ID3D10Buffer* resource_{nullptr};
  void*         mapping_{nullptr};

private:
  XRAY_NO_COPY(scoped_buffer_mapping);
};

} // namespace directx10
} // namespace rendering
} // namespace xray
