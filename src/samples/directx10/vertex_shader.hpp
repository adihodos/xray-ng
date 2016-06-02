#pragma once

#include "xray/xray.hpp"
#include "gpu_shader.hpp"
#include "gpu_shader_traits.hpp"
#include <cassert>
#include <d3d10.h>

namespace xray {
namespace rendering {

#if defined(XRAY_RENDERER_DIRECTX)
inline namespace directx10 {
#else
namespace directx10 {
#endif

class renderer;

template <>
struct shader_traits<shader_type::vertex> {
  using handle_type        = ID3D10VertexShader*;
  using scoped_shader_type = base::com_ptr<ID3D10VertexShader>;

  static scoped_shader_type create(ID3D10Device* device, ID3D10Blob* blob) {
    scoped_shader_type new_shader;
    device->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(),
                               raw_ptr_ptr(new_shader));

    return new_shader;
  }

  static void bind(ID3D10Device* device, ID3D10VertexShader* shader) noexcept {
    device->VSSetShader(shader);
  }

  static void set_constant_buffer(ID3D10Device*        device,
                                  const uint32_t       start_slot,
                                  const uint32_t       num_buffers,
                                  ID3D10Buffer* const* buffers) {
    device->VSSetConstantBuffers(start_slot, num_buffers, buffers);
  }
};

using vertex_shader = gpu_shader<shader_type::vertex>;

} // namespace directx10
} // namespace rendering
} // namespace xray
