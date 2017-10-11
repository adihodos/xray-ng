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
#include "xray/math/scalar3.hpp"
#include "xray/rendering/colors/rgb_color.hpp"

namespace app {

/// \brief  A light source.
struct light_source {
  xray::math::vec3f position;

  ///<  Padding to mirror the GLSL layout.
  float pad1;

  ///< Ambient component.
  xray::rendering::rgb_color ka;

  ///< Diffuse component.
  xray::rendering::rgb_color kd;

  ///< Specular component.
  xray::rendering::rgb_color ke;
};

struct light_source2 {
  xray::math::vec3f position;

  ///<  Padding to mirror the GLSL layout.
  float pad1;

  ///<  Light intensity.
  xray::math::vec3f intensity;

  float pad2;

  ///< Ambient component.
  xray::rendering::rgb_color ka;

  ///< Diffuse component.
  xray::rendering::rgb_color kd;

  ///< Specular component.
  xray::rendering::rgb_color ks;
};

// struct directional_light {
//  xray::math::vec3f          direction;
//  float                      pad1;
//  xray::rendering::rgb_color ka;
//  xray::rendering::rgb_color kd;
//  xray::rendering::rgb_color ks;
//};

struct light_source3 {
  xray::math::vec3f position;
  float             pad1;
  xray::math::vec3f intensity;
};

struct light_source4 {
  xray::rendering::rgb_color color;
  xray::math::vec3f          position;
};

struct spotlight {
  xray::math::vec3f          position;
  float                      cutoff_angle;
  xray::math::vec3f          direction;
  float                      light_power;
  xray::rendering::rgb_color ka;
  xray::rendering::rgb_color kd;
  xray::rendering::rgb_color ks;
};

} // namespace app
