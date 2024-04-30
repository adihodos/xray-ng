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

//
//  All code in this file was automatically generated, DO NOT EDIT !!!

#include <cassert>
#include <cstdint>
#include <iterator>

namespace app {

struct phong_material_component
{
  private:
    phong_material_component() = delete;
    ~phong_material_component() = delete;

  public:
    enum class e : uint8_t
    {
        emissive,
        ambient,
        diffuse,
        specular,
        specular_intensity
    };

    using underlying_type = uint8_t;

    static const char* qualified_name(const phong_material_component::e member) noexcept;
    static const char* name(const phong_material_component::e member) noexcept;

    static const char* class_name() noexcept { return "phong_material_component::e"; }

    static underlying_type to_integer(const phong_material_component::e member) noexcept
    {
        return static_cast<underlying_type>(member);
    }

    static phong_material_component::e from_integer(const underlying_type ival) noexcept
    {
        return static_cast<phong_material_component::e>(ival);
    }

    using const_iterator = const e*;
    using reverse_const_iterator = std::reverse_iterator<const_iterator>;

    static constexpr size_t size = 5u;
    static const_iterator cbegin() noexcept { return &_member_entries[0]; }
    static const_iterator cend() noexcept { return &_member_entries[5]; }
    static reverse_const_iterator crbegin() noexcept { return reverse_const_iterator(cend()); }
    static reverse_const_iterator crend() noexcept { return reverse_const_iterator(cbegin()); }
    static bool is_defined(const phong_material_component::e val) noexcept;

  private:
    static constexpr const e _member_entries[] = { e::emissive,
                                                   e::ambient,
                                                   e::diffuse,
                                                   e::specular,
                                                   e::specular_intensity };
};

} // end ns