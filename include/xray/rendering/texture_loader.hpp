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

#include "xray/base/unique_pointer.hpp"
#include "xray/xray.hpp"
#include <cstdint>
#include <opengl/opengl.hpp>
#include <string>

namespace xray {
namespace rendering {

namespace detail {

struct stbi_img_deleter
{
    void operator()(uint8_t* mem) const noexcept;
};

} // namespace detail

enum class texture_load_options : uint32_t
{
    none = 0U,

    ///< Flip image vertically on load
    flip_y = 1U << 0
};

class texture_loader
{
  public:
    texture_loader() = default;
    ~texture_loader();

    explicit texture_loader(const std::string& file_path,
                            const texture_load_options load_opts = texture_load_options::none)
        : texture_loader{ file_path.c_str(), load_opts }
    {
    }

    explicit texture_loader(const char* file_path, const texture_load_options load_opts = texture_load_options::none);

    texture_loader& operator=(texture_loader&&) = default;
    texture_loader(texture_loader&&) = default;

  public:
    explicit operator bool() const noexcept { return _texdata != nullptr; }

    int32_t width() const noexcept { return _x_size; }

    int32_t height() const noexcept { return _y_size; }

    int32_t depth() const noexcept { return _levels; }

    GLenum format() const noexcept;

    GLenum internal_format() const noexcept;

    GLenum pixel_size() const noexcept { return gl::UNSIGNED_BYTE; }

    const uint8_t* data() const noexcept { return xray::base::raw_ptr(_texdata); }

  private:
    xray::base::unique_pointer<uint8_t, detail::stbi_img_deleter> _texdata;
    int32_t _x_size{};
    int32_t _y_size{};
    int32_t _levels{};

  private:
    XRAY_NO_COPY(texture_loader);
};

} // namespace rendering
} // namespace xray
