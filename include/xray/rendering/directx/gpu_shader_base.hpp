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
#include <cassert>
#include <cstdint>
#include <d3d11.h>
#include <d3d11shader.h>
#include <string>
#include <utility>
#include <vector>

namespace xray {
namespace rendering {

/// \addtogroup __GroupXrayRendering_Directx
/// @{

namespace detail {

struct shader_resource_view {
  std::string               srv_name;
  uint32_t                  srv_bindpoint{};
  uint32_t                  srv_bindcount{};
  D3D11_SRV_DIMENSION       srv_dimension{D3D11_SRV_DIMENSION_UNKNOWN};
  ID3D11ShaderResourceView* srv_bound_view{};
};

struct sampler_state {
  std::string         ss_name;
  uint32_t            ss_bindpoint{};
  ID3D11SamplerState* ss_handle{};
};

/// Stores information about a uniform block defined in a shader.
struct shader_uniform_block {
  uint32_t sub_store_offset{};

  ///< Block's name.
  std::string sub_name;

  uint32_t sub_hashed_name{};

  ///< Block variable count.
  uint32_t sub_var_count{0};

  ///< Block's size, in bytes.
  uint32_t sub_bytes_size{0};

  ///< Binding point (GPU register).
  uint32_t sub_gpu_bindpoint{0};

  ///< True if data in CPU memory is out of sync with data in GPU memory.
  mutable bool sub_dirty{false};

  ///< Handle to constant buffer in GPU memory.
  base::com_ptr<ID3D11Buffer> sub_gpu_buffer;

public:
  /// \name Constructors
  /// @{

  shader_uniform_block()                              = default;
  shader_uniform_block(shader_uniform_block&& rvalue) = default;

  shader_uniform_block(ID3D11Device*                   device_context,
                       const D3D11_SHADER_BUFFER_DESC& buff_desc,
                       const uint32_t                  bind_point);

  /// @}

  /// \name Operators
  ///

  shader_uniform_block& operator=(shader_uniform_block&& rvalue) = default;
  explicit operator bool() const noexcept { return sub_gpu_buffer != nullptr; }

  /// @}

private:
  XRAY_NO_COPY(shader_uniform_block);
};

///
/// Stores information abount a shader uniform.
struct shader_uniform {

  /// Hashed name of the parent block.
  uint32_t su_parent_block_id{};

  /// Uniform's name.
  std::string su_name;

  uint32_t su_hashed_name{};

  /// Uniform's offset in bytes, from the start of the parent block.
  uint32_t su_offset_in_parent{};

  /// Uniform's size in bytes.
  uint32_t su_bytes_size{};

  /// \name Constructors
  /// @{

  shader_uniform() = default;

  shader_uniform(const D3D11_SHADER_VARIABLE_DESC& var_desc,
                 const uint32_t                    parent_blk_id)
      : su_parent_block_id{parent_blk_id}
      , su_name{var_desc.Name}
      , su_hashed_name{FNV::fnv1a(var_desc.Name)}
      , su_offset_in_parent{var_desc.StartOffset}
      , su_bytes_size{var_desc.Size} {}

  /// @}
};

} // namespace detail

struct shader_common_core {
public:
  shader_common_core()                     = default;
  shader_common_core(shader_common_core&&) = default;
  shader_common_core& operator=(shader_common_core&&) = default;
  explicit shader_common_core(ID3D11Device* device,
                              ID3D10Blob*   compiled_bytecode);

  ~shader_common_core();

  struct start_slots {
    uint32_t ss_cbuffer{};
    uint32_t ss_resourceviews{};
    uint32_t ss_samplers{};
  };

  void set_uniform_block(const char* block_name, const void* block_data,
                         const size_t bytes_count) noexcept;
  void set_uniform(const char* uniform_name, const void* uniform_data,
                   const size_t bytes_count) noexcept;

  void set_resource_view(const char*               rv_name,
                         ID3D11ShaderResourceView* rv_handle);

  void set_sampler(const char* sampler_name, ID3D11SamplerState* sampler_state);

  void set_constant_buffers(ID3D11DeviceContext* device_context,
                            void (ID3D11DeviceContext::*cbuff_set_fn)(
                                UINT, UINT, ID3D11Buffer* const*));

  void set_resource_views(ID3D11DeviceContext* device_context,
                          void (ID3D11DeviceContext::*rvs_set_fn)(
                              UINT, UINT, ID3D11ShaderResourceView* const*));

  void set_samplers(ID3D11DeviceContext* device_context,
                    void (ID3D11DeviceContext::*samplers_set_fn)(
                        UINT, UINT, ID3D11SamplerState* const*));

  explicit operator bool() const noexcept { return _valid; }

public:
  std::vector<detail::shader_uniform_block> _uniform_blocks;
  std::vector<detail::shader_uniform>       _uniforms;
  std::vector<detail::shader_resource_view> _resource_views;
  std::vector<detail::sampler_state>        _samplers;
  xray::base::unique_pointer<uint8_t[]>     _datastore;
  start_slots                               _st_slots;
  bool                                      _valid{false};

private:
  XRAY_NO_COPY(shader_common_core);
};

/// @}

} // namespace rendering
} // namespace xray
