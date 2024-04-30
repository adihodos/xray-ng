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

#include "demo_base.hpp"
#include "light_source.hpp"
#include "material.hpp"
#include "xray/math/scalar2.hpp"
#include "xray/math/scalar3.hpp"
#include "xray/rendering/colors/rgb_color.hpp"
#include "xray/rendering/mesh.hpp"
#include "xray/rendering/opengl/gl_handles.hpp"
#include "xray/rendering/opengl/gpu_program.hpp"
#include "xray/xray.hpp"

namespace app {

class refraction_demo : public demo_base
{
  public:
    refraction_demo();

    ~refraction_demo();

    void compose_ui();

    virtual void draw(const xray::rendering::draw_context_t&) override;

    virtual void update(const float delta_ms) override;

    virtual void key_event(const int32_t key_code, const int32_t action, const int32_t mods) override;

    explicit operator bool() const noexcept { return valid(); }

  private:
    void init();

  private:
    xray::rendering::gpu_program _drawprogram;
    xray::rendering::scoped_buffer _vertexbuffer_skybox;
    xray::rendering::scoped_buffer _indexbuffer_skybox;
    xray::rendering::scoped_vertex_array _vertexarray_skybox;
    uint32_t _skybox_indexcount{};
    xray::rendering::simple_mesh _spacecraft;
    xray::rendering::scoped_texture _skybox_texture;
    xray::rendering::scoped_sampler _skybox_sampler;
    bool _skybox_wiremesh{ false };

    struct refraction_params_t
    {
        float refract_surface{ 0.9f };
        float reflect_surface{ 0.1f };
    } _surfaceparams;

  private:
    XRAY_NO_COPY(refraction_demo);
};

} // namespace app
