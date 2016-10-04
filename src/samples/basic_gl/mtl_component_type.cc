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

#include "mtl_component_type.hpp"
#include "xray/xray.hpp"
#include <algorithm>

namespace app {

constexpr mtl_component_type::e mtl_component_type::_member_entries[];

const char* mtl_component_type::to_string(const mtl_component_type::e member) {
  switch (member) {
  case mtl_component_type::e::emissive:
    return "mtl_component_type::e::emissive";
    break;
  case mtl_component_type::e::ambient:
    return "mtl_component_type::e::ambient";
    break;
  case mtl_component_type::e::diffuse:
    return "mtl_component_type::e::diffuse";
    break;
  case mtl_component_type::e::specular:
    return "mtl_component_type::e::specular";
    break;

  default:
    assert(false && "Unknown enum member!");
    break;
  }

  return "error/unknown";
}

const char* mtl_component_type::name(const mtl_component_type::e member) {
  switch (member) {
  case mtl_component_type::e::emissive:
    return "emissive";
    break;
  case mtl_component_type::e::ambient:
    return "ambient";
    break;
  case mtl_component_type::e::diffuse:
    return "diffuse";
    break;
  case mtl_component_type::e::specular:
    return "specular";
    break;

  default:
    assert(false && "Unknown enum member!");
    break;
  }

  return "error/unknown";
}

bool mtl_component_type::is_defined(const mtl_component_type::e val) {
  using namespace std;
  return find(mtl_component_type::cbegin(), mtl_component_type::cend(), val) !=
         mtl_component_type::cend();
}

} // end ns