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
#include "xray/rendering/colors/rgb_color.hpp"

namespace app {

struct material {
  ///<  Emissive component.
  xray::rendering::rgb_color ke;

  ///<  Ambient component.
  xray::rendering::rgb_color ka;

  ///<  Diffuse component.
  xray::rendering::rgb_color kd;

  ///<  Specular component.
  xray::rendering::rgb_color ks;

  struct stdc;
};

struct material::stdc {
  static constexpr material copper{{0.0f, 0.0f, 0.0f},
                                   {0.2295f, 0.08825f, 0.0275f},
                                   {0.5508f, 0.2118f, 0.066f},
                                   {0.580594f, 0.223257f, 0.0695701f, 51.2f}};
  static constexpr material gold{{0.0f, 0.0f, 0.0f},
                                 {0.24725f, 0.2245f, 0.0645f},
                                 {0.34615f, 0.3143f, 0.0903f},
                                 {0.797357f, 0.723991f, 0.208006f, 83.2f}};

  static constexpr material brass{
      {0.0f, 0.0f, 0.0f, 0.0f},
      {0.329412f, 0.223529f, 0.027451f, 1.0f},
      {0.780392f, 0.568627f, 0.113725f, 1.0f},
      {0.992157f, 0.941176f, 0.807843f, 128.0f * 0.21794872f}};

  static constexpr material chrome{
      {0.0f, 0.0f, 0.0f, 0.0f},
      {0.25f, 0.25f, 0.25f, 1.0f},
      {0.4f, 0.4f, 0.4f, 1.0f},
      {0.774597f, 0.774597f, 0.774597f, 128.0f * 0.6}};

  static constexpr material silver{
      {0.0f, 0.0f, 0.0f, 0.0f},
      {0.19225f, 0.19225f, 0.19225f, 1.0f},
      {0.50754f, 0.50754f, 0.50754f, 1.0f},
      {0.508273f, 0.508273f, 0.508273f, 128.0f * 0.4f}};

  static constexpr material green_plastic{
      {0.0f, 0.0f, 0.0f, 0.0f},
      {0.0f, 0.0f, 0.0f, 1.0f},
      {0.1f, 0.35f, 0.1f, 1.0f},
      {0.45f, 0.55f, 0.45f, 128.0f * 0.25f}};

  static constexpr material red_plastic{{0.0f, 0.0f, 0.0f, 0.0f},
                                        {0.0f, 0.0f, 0.0f, 1.0f},
                                        {0.5f, 0.0f, 0.0f, 1.0f},
                                        {0.7f, 0.6f, 0.6f, 128.0f * 0.25f}};
};

struct material_ad {
  xray::rendering::rgb_color ka;
  xray::rendering::rgb_color kd;

  struct stdc;
};

struct material_ad::stdc {
  static constexpr material_ad copper{{0.2295f, 0.08825f, 0.0275f},
                                      {0.5508f, 0.2118f, 0.066f}};

  static constexpr material_ad gold{{0.24725f, 0.2245f, 0.0645f},
                                    {0.34615f, 0.3143f, 0.0903f}};
};

} // namespace app
