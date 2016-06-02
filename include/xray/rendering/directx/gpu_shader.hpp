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

/// \file gpu_shader.hpp

#include "xray/xray.hpp"
#include "xray/base/fnv_hash.hpp"
#include "xray/base/windows/com_ptr.hpp"
#include "xray/rendering/directx/gpu_shader_base.hpp"
#include <cstdint>
#include <d3d11.h>
#include <d3dcompiler.h>

namespace xray {
namespace rendering {

/// \addtogroup __GroupXrayRendering_Directx
/// @{

base::com_ptr<ID3D10Blob> compile_shader(const char*    src_code,
                                         const uint32_t code_len,
                                         const char*    entry_pt,
                                         const char*    target,
                                         const uint32_t compile_flags);

base::com_ptr<ID3D10Blob> compile_shader(const char* file, const char* entry_pt,
                                         const char*    target,
                                         const uint32_t compile_flags);

enum class shader_compile_flags : uint32_t {
  debug = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION |
          D3DCOMPILE_OPTIMIZATION_LEVEL0 | D3DCOMPILE_ENABLE_STRICTNESS |
          D3DCOMPILE_WARNINGS_ARE_ERRORS,

  release = D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_OPTIMIZATION_LEVEL2 |
            D3DCOMPILE_WARNINGS_ARE_ERRORS
};

template <typename shader_traits>
class gpu_shader {
public:
  using interface_type = typename shader_traits::interface_type;
  using handle_type    = typename shader_traits::handle_type;

  gpu_shader()                            = default;
  ~gpu_shader()                           = default;
  gpu_shader(gpu_shader<shader_traits>&&) = default;
  gpu_shader<shader_traits>& operator=(gpu_shader<shader_traits>&&) = default;

  gpu_shader(
      ID3D11Device* device, const char* src_code, const uint32_t code_len,
      const char*                entry_point,
      const shader_compile_flags compile_flags = shader_compile_flags::release);

  gpu_shader(
      ID3D11Device* device, const char* shader_file, const char* entry_point,
      const shader_compile_flags compile_flags = shader_compile_flags::release);

  explicit operator bool() const noexcept { return valid(); }

  bool valid() const noexcept {
    return (_bytecode != nullptr) && _shader_core._valid &&
           (_shader != nullptr);
  }

  handle_type handle() const noexcept { return xray::base::raw_ptr(_shader); }

  ID3D10Blob* bytecode() const noexcept {
    return xray::base::raw_ptr(_bytecode);
  }

  void bind(ID3D11DeviceContext* device_context);

public:
  template <typename T>
  void set_uniform_block(const char* name, const T& data) noexcept {
    assert(valid());
    set_uniform_block(name, &data, sizeof(data));
  }

  void set_uniform_block(const char* name, const void* data,
                         const size_t byte_size) noexcept {
    assert(valid());
    _shader_core.set_uniform_block(name, data, byte_size);
  }

  template <typename T>
  void set_uniform(const char* name, const T& data) noexcept {
    assert(valid());
    set_uniform(name, &data, sizeof(data));
  }

  void set_uniform(const char* name, const void* data,
                   const size_t byte_size) noexcept {
    assert(valid());
    _shader_core.set_uniform(name, data, byte_size);
  }

  void set_resource_view(const char*               rv_name,
                         ID3D11ShaderResourceView* rv_handle) {
    assert(valid());
    _shader_core.set_resource_view(rv_name, rv_handle);
  }

  void set_sampler(const char* sampler_name, ID3D11SamplerState* state_handle) {
    assert(valid());
    _shader_core.set_sampler(sampler_name, state_handle);
  }

private:
  xray::base::com_ptr<ID3D10Blob>     _bytecode;
  shader_common_core                  _shader_core;
  xray::base::com_ptr<interface_type> _shader;

private:
  XRAY_NO_COPY(gpu_shader);
};

template <typename shader_traits>
gpu_shader<shader_traits>::gpu_shader(ID3D11Device*              device,
                                      const char*                src_code,
                                      const uint32_t             code_len,
                                      const char*                entry_point,
                                      const shader_compile_flags compile_flags)
    : _bytecode{compile_shader(src_code, code_len, entry_point,
                               shader_traits::target,
                               static_cast<uint32_t>(compile_flags))}
    , _shader_core{device, raw_ptr(_bytecode)} {
  if (!_bytecode || !_shader_core)
    return;

  xray::base::unique_pointer_reset(
      _shader, shader_traits::create(device, raw_ptr(_bytecode)));
}

template <typename shader_traits>
gpu_shader<shader_traits>::gpu_shader(ID3D11Device*              device,
                                      const char*                shader_file,
                                      const char*                entry_point,
                                      const shader_compile_flags compile_flags)
    : _bytecode{compile_shader(shader_file, entry_point, shader_traits::target,
                               static_cast<uint32_t>(compile_flags))}
    , _shader_core{device, raw_ptr(_bytecode)} {

  if (!_bytecode || !_shader_core)
    return;

  xray::base::unique_pointer_reset(
      _shader, shader_traits::create(device, raw_ptr(_bytecode)));
}

template <typename shader_traits>
inline void
gpu_shader<shader_traits>::bind(ID3D11DeviceContext* device_context) {
  assert(valid());
  using namespace std;

  _shader_core.set_constant_buffers(device_context,
                                    shader_traits::set_constant_buffers);

  _shader_core.set_resource_views(device_context,
                                  shader_traits::set_resource_views);

  _shader_core.set_samplers(device_context, shader_traits::set_samplers);

  shader_traits::bind(device_context, handle());
}

/// @}

} // namespace rendering
} // namespace xray
