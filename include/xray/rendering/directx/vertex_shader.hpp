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
#include "xray/rendering/directx/gpu_shader.hpp"
#include <cstdint>
#include <d3d11.h>

namespace xray {
namespace rendering {

/// \addtogroup __GroupXrayRendering_Directx
/// @{

struct vertex_shader_traits {
  using interface_type = ID3D11VertexShader;
  using handle_type    = ID3D11VertexShader*;

  static constexpr const char* const target = "vs_5_0";

  static handle_type create(ID3D11Device* device,
                            ID3D10Blob*   compiled_bytecode) {
    ID3D11VertexShader* vs{};
    device->CreateVertexShader(compiled_bytecode->GetBufferPointer(),
                               compiled_bytecode->GetBufferSize(), nullptr,
                               &vs);
    return vs;
  }

  static void bind(ID3D11DeviceContext* context, ID3D11VertexShader* shader) {
    context->VSSetShader(shader, nullptr, 0);
  }

  static constexpr auto set_constant_buffers =
      &ID3D11DeviceContext::VSSetConstantBuffers;

  static constexpr auto set_resource_views =
      &ID3D11DeviceContext::VSSetShaderResources;

  static constexpr auto set_samplers = &ID3D11DeviceContext::VSSetSamplers;
};

using vertex_shader = gpu_shader<vertex_shader_traits>;

/// @}

} // namespace rendering
} // namespace xray
