//
// Copyright (c) 2011, 2012, 2013 Adrian Hodos
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
#include <cstdint>
#include <d3d11.h>

namespace xray {
namespace rendering {

/// \addtogroup __GroupXrayRendering_Directx
/// @{

struct scoped_resource_mapping {
public:
  scoped_resource_mapping(ID3D11DeviceContext* dc, ID3D11Resource* res,
                          const D3D11_MAP map_type,
                          const uint32_t  subres_idx = 0);

  ~scoped_resource_mapping();

  void* memory() noexcept { return _mapping.pData; }

  const void* memory() const noexcept { return _mapping.pData; }

  explicit operator bool() const noexcept { return _mapping.pData != nullptr; }

private:
  ID3D11DeviceContext*     _devctx;
  ID3D11Resource*          _resource;
  uint32_t                 _subresource{};
  D3D11_MAPPED_SUBRESOURCE _mapping{nullptr, 0, 0};

private:
  XRAY_NO_COPY(scoped_resource_mapping);
};

/// @}

} // namespace rendering
} // namespace xray
