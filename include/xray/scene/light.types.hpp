//
// Copyright (c) Adrian Hodos
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

#include "xray/math/scalar3.hpp"
#include "xray/math/scalar4.hpp"

namespace xray::scene {

struct DirectionalLight
{
    alignas(16) math::vec3f direction;
    alignas(16) math::vec4f ambient;
    alignas(16) math::vec4f diffuse;
    alignas(16) math::vec4f specular;
};

struct PointLight
{
    math::vec3f position;
    float range;
    alignas(16) math::vec3f attenuation;
    alignas(16) math::vec4f ambient;
    alignas(16) math::vec4f diffuse;
    alignas(16) math::vec4f specular;
};

struct SpotLight
{
    math::vec3f position;
    float range;
    math::vec3f direction;
    float spot;
    alignas(16) math::vec3f attenuation;
    alignas(16) math::vec4f ambient;
    alignas(16) math::vec4f diffuse;
    alignas(16) math::vec4f specular;
};

}