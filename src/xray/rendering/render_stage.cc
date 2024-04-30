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

#include "xray/rendering/render_stage.hpp"
#include "xray/xray.hpp"
#include <algorithm>

namespace xray {
namespace rendering {

constexpr const render_stage::e render_stage::_member_entries[];

const char*
render_stage::qualified_name(const render_stage::e member) noexcept
{
    switch (member) {
        case render_stage::e::vertex:
            return "render_stage::e::vertex";
            break;
        case render_stage::e::tess_control:
            return "render_stage::e::tess_control";
            break;
        case render_stage::e::tess_eval:
            return "render_stage::e::tess_eval";
            break;
        case render_stage::e::geometry:
            return "render_stage::e::geometry";
            break;
        case render_stage::e::fragment:
            return "render_stage::e::fragment";
            break;
        case render_stage::e::compute:
            return "render_stage::e::compute";
            break;

        default:
            assert(false && "Unknown enum member!");
            break;
    }

    return "unknown enum member";
}

const char*
render_stage::name(const render_stage::e member) noexcept
{
    switch (member) {
        case render_stage::e::vertex:
            return "vertex";
            break;
        case render_stage::e::tess_control:
            return "tess_control";
            break;
        case render_stage::e::tess_eval:
            return "tess_eval";
            break;
        case render_stage::e::geometry:
            return "geometry";
            break;
        case render_stage::e::fragment:
            return "fragment";
            break;
        case render_stage::e::compute:
            return "compute";
            break;

        default:
            assert(false && "Unknown enum member");
            break;
    }

    return "unknown enum member";
}

bool
render_stage::is_defined(const render_stage::e val) noexcept
{
    using namespace std;
    return find(render_stage::cbegin(), render_stage::cend(), val) != render_stage::cend();
}

} // namespace rendering
} // namespace xray
