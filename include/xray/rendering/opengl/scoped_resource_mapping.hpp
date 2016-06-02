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

#include "xray/xray.hpp"
#include "xray/base/debug/debug_ext.hpp"
#include "xray/base/logger.hpp"
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <opengl/opengl.hpp>
#include <string>
#include <vector>

namespace xray {
namespace rendering {

class scoped_resource_mapping {
public:
  scoped_resource_mapping(const uint32_t resource, const uint32_t access,
                          const uint32_t length, const uint32_t offset = 0)
      : gl_resource_{resource} {

    mapping_ptr_ = gl::MapNamedBufferRange(resource, offset, length, access);

    if (!mapping_ptr_) {
      XR_LOG_ERR("Failed to map buffer into client memory!");
    }
  }

  ~scoped_resource_mapping() {
    if (mapping_ptr_) {
      const auto result = gl::UnmapNamedBuffer(gl_resource_);
      if (result != gl::TRUE_)
        XR_LOG_ERR("Failed to unmap buffer !");
    }
  }

  bool valid() const noexcept { return mapping_ptr_ != nullptr; }

  explicit operator bool() const noexcept { return valid(); }

  void* memory() const noexcept { return mapping_ptr_; }

private:
  void*    mapping_ptr_{nullptr};
  uint32_t gl_resource_{0};

private:
  XRAY_NO_COPY(scoped_resource_mapping);
  XRAY_NO_MOVE(scoped_resource_mapping);
};

} // namespace rendering
} // namespace xray
