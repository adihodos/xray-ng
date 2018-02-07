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
#include "xray/base/windows/com_ptr.hpp"
#include <d3d11.h>

namespace xray {
namespace rendering {

/// \addtogroup __GroupXrayRendering_Directx
/// @{

class scoped_rasterizer_state {
public:
  explicit scoped_rasterizer_state(ID3D11DeviceContext*   context,
                                   ID3D11RasterizerState* new_state) noexcept
    : _context{context} {
    context->RSGetState(&_old_state);
    context->RSSetState(new_state);
  }

  ~scoped_rasterizer_state() noexcept { _context->RSSetState(_old_state); }

private:
  ID3D11DeviceContext*   _context{};
  ID3D11RasterizerState* _old_state{};

private:
  XRAY_NO_COPY(scoped_rasterizer_state);
};

class scoped_blend_state {
public:
  explicit scoped_blend_state(ID3D11DeviceContext* context,
                              ID3D11BlendState*    new_state,
                              const float*         blend_factors,
                              const uint32_t       sample_mask) noexcept;

  ~scoped_blend_state() noexcept {
    _context->OMSetBlendState(_saved_state, _blend_factors, _mask);
  }

private:
  ID3D11DeviceContext* _context;
  ID3D11BlendState*    _saved_state;
  float                _blend_factors[4];
  uint32_t             _mask;

private:
  XRAY_NO_COPY(scoped_blend_state);
};

class scoped_depth_stencil_state {
public:
  explicit scoped_depth_stencil_state(ID3D11DeviceContext*     context,
                                      ID3D11DepthStencilState* new_state,
                                      const uint32_t stencil_ref) noexcept;

  ~scoped_depth_stencil_state() noexcept {
    _context->OMSetDepthStencilState(_saved_state, _saved_stencil_ref);
  }

private:
  ID3D11DeviceContext*     _context;
  ID3D11DepthStencilState* _saved_state{};
  uint32_t                 _saved_stencil_ref{};

private:
  XRAY_NO_COPY(scoped_depth_stencil_state);
};

class scoped_viewport {
public:
  scoped_viewport(ID3D11DeviceContext*  context,
                  const D3D11_VIEWPORT* viewports,
                  const size_t          num_viewports) noexcept;

  ~scoped_viewport() noexcept;

private:
  ID3D11DeviceContext* _context;
  D3D11_VIEWPORT
  _saved_viewport[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE];
  uint32_t _viewport_count{
    D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE};

private:
  XRAY_NO_COPY(scoped_viewport);
};

class scoped_scissor_rects {
public:
  scoped_scissor_rects(ID3D11DeviceContext* context,
                       const D3D11_RECT*    scissor_rects,
                       const size_t         num_rects) noexcept;

  ~scoped_scissor_rects() noexcept;

private:
  ID3D11DeviceContext* _context;
  D3D11_RECT           _saved_scissor_rects
    [D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE];
  uint32_t _scissors_count{
    D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE};

private:
  XRAY_NO_COPY(scoped_scissor_rects);
};

/// @}

} // namespace rendering
} // namespace xray
