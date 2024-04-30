#pragma once

#include "xray/math/scalar3.hpp"
#include "xray/rendering/colors/rgb_color.hpp"
#include "xray/rendering/directx/pixel_shader.hpp"
#include "xray/rendering/directx/vertex_shader.hpp"
#include "xray/rendering/rendering_fwd.hpp"
#include "xray/xray.hpp"
#include <cstdint>
#include <d3d11.h>

namespace app {

struct directional_light
{
    xray::math::vec3f direction;
    float pad1;
    xray::rendering::rgb_color color;
};

struct simple_object
{
  public:
    simple_object(ID3D11Device* device);

    ~simple_object();

    explicit operator bool() const noexcept { return valid(); }

    bool valid() const noexcept { return _valid; }

    void draw(const xray::rendering::draw_context_t&);

  private:
    void init(ID3D11Device* device);

  private:
    xray::base::com_ptr<ID3D11Buffer> _vertex_buffer;
    xray::base::com_ptr<ID3D11Buffer> _index_buffer;
    xray::base::com_ptr<ID3D11InputLayout> _layout;
    xray::base::com_ptr<ID3D11RasterizerState> _rasterstate;
    xray::rendering::vertex_shader _vertex_shader;
    xray::rendering::pixel_shader _pixel_shader;
    xray::base::com_ptr<ID3D11Buffer> _cbuff_ps;
    xray::base::com_ptr<ID3D11ShaderResourceView> _diffuse_mat;
    xray::base::com_ptr<ID3D11SamplerState> _sampler;
    uint32_t _index_count{};
    bool _valid{ false };
    directional_light _light;

  private:
    XRAY_NO_COPY(simple_object);
};
} // namespace app
