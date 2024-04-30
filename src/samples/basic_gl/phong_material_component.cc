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

//
//  All code in this file was automatically generated, DO NOT EDIT !!!

#include "phong_material_component.hpp"
#include "xray/xray.hpp"
#include <algorithm>

namespace app {

constexpr const phong_material_component::e phong_material_component::_member_entries[];

const char*
phong_material_component::qualified_name(const phong_material_component::e member) noexcept
{
    switch (member) {
        case phong_material_component::e::emissive:
            return "phong_material_component::e::emissive";
            break;
        case phong_material_component::e::ambient:
            return "phong_material_component::e::ambient";
            break;
        case phong_material_component::e::diffuse:
            return "phong_material_component::e::diffuse";
            break;
        case phong_material_component::e::specular:
            return "phong_material_component::e::specular";
            break;
        case phong_material_component::e::specular_intensity:
            return "phong_material_component::e::specular_intensity";
            break;

        default:
            assert(false && "Unknown enum member!");
            break;
    }

    return "unknown enum member";
}

const char*
phong_material_component::name(const phong_material_component::e member) noexcept
{
    switch (member) {
        case phong_material_component::e::emissive:
            return "emissive";
            break;
        case phong_material_component::e::ambient:
            return "ambient";
            break;
        case phong_material_component::e::diffuse:
            return "diffuse";
            break;
        case phong_material_component::e::specular:
            return "specular";
            break;
        case phong_material_component::e::specular_intensity:
            return "specular_intensity";
            break;

        default:
            assert(false && "Unknown enum member");
            break;
    }

    return "unknown enum member";
}

bool
phong_material_component::is_defined(const phong_material_component::e val) noexcept
{
    using namespace std;
    return find(phong_material_component::cbegin(), phong_material_component::cend(), val) !=
           phong_material_component::cend();
}

} // end ns