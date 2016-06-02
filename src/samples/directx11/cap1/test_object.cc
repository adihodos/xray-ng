#include "cap1/test_object.hpp"
#include "xray/base/app_config.hpp"
#include "xray/base/logger.hpp"
#include "xray/math/constants.hpp"
#include "xray/math/scalar2.hpp"
#include "xray/math/scalar3.hpp"
#include "xray/math/scalar3_math.hpp"
#include "xray/math/scalar4.hpp"
#include "xray/math/scalar4x4_math.hpp"
#include "xray/rendering/colors/color_palettes.hpp"
#include "xray/rendering/directx/DDSTextureLoader.h"
#include "xray/rendering/directx/WICTextureLoader.h"
#include "xray/rendering/directx/dx11_renderer.hpp"
#include "xray/rendering/directx/scoped_mapping.hpp"
#include "xray/rendering/draw_context.hpp"
#include "xray/rendering/geometry/geometry_data.hpp"
#include "xray/rendering/geometry/geometry_factory.hpp"
#include "xray/rendering/vertex_format/vertex_pnt.hpp"
#include <algorithm>
#include <d3dcompiler.h>
#include <platformstl/filesystem/memory_mapped_file.hpp>
#include <span.h>
#include <vector>

using namespace xray::base;
using namespace xray::math;
using namespace xray::rendering;
using namespace std;

struct ripple_params {
  float amplitude{};
  float period{};
  float grid_width{};
  float grid_depth{};
};

void ripple_grid(xray::rendering::geometry_data_t* mesh,
                 const ripple_params&              rp) noexcept {

  assert(mesh != nullptr);

  using namespace xray::rendering;
  using namespace xray::base;

  auto vertices = gsl::span<vertex_pntt>{
      raw_ptr(mesh->geometry), raw_ptr(mesh->geometry) + mesh->vertex_count};

  const auto scale_x = rp.grid_width * 0.5f;
  const auto scale_z = rp.grid_depth * 0.5f;

  for (auto& vertex : vertices) {
    vertex.normal    = float3::stdc::zero;
    const auto x_pos = vertex.position.x / scale_x;
    const auto z_pos = vertex.position.z / scale_z;

    vertex.position.y =
        rp.amplitude * std::sin(rp.period * (x_pos * x_pos + z_pos * z_pos));
  }

  const auto indices = gsl::span<uint32_t>{
      raw_ptr(mesh->indices), static_cast<ptrdiff_t>(mesh->index_count)};

  assert((indices.length() % 3) == 0);

  for (uint32_t i = 0, idx_count = static_cast<uint32_t>(indices.length() / 3);
       i < idx_count; i += 3) {
    auto& v0 = vertices[indices[i + 0]];
    auto& v1 = vertices[indices[i + 1]];
    auto& v2 = vertices[indices[i + 2]];

    const auto normal =
        cross(v2.position - v0.position, v1.position - v0.position);

    v0.normal += normal;
    v1.normal += normal;
    v2.normal += normal;
  }

  for (auto& vertex : vertices) {
    vertex.normal = normalize(vertex.normal);
  }
}

app::simple_object::simple_object(ID3D11Device* device) { init(device); }

app::simple_object::~simple_object() {}

void app::simple_object::draw(const xray::rendering::draw_context_t& dc) {
  assert(valid());

  const auto renderer = static_cast<const dx11_renderer*>(dc.renderer);

  {
    ID3D11Buffer* vertex_buff = raw_ptr(_vertex_buffer);
    uint32_t      stride      = static_cast<uint32_t>(sizeof(vertex_pnt));
    uint32_t      offset      = 0;

    renderer->context()->IASetVertexBuffers(0, 1, &vertex_buff, &stride,
                                            &offset);
    renderer->context()->IASetIndexBuffer(raw_ptr(_index_buffer),
                                          DXGI_FORMAT_R32_UINT, 0);
    renderer->context()->IASetInputLayout(raw_ptr(_layout));
    renderer->context()->IASetPrimitiveTopology(
        D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  }

  {
    struct matrix_pack {
      float4x4 world_to_view;
      float4x4 normals_to_view;
      float4x4 world_view_proj;
    } const matrices{dc.view_matrix, dc.view_matrix, dc.proj_view_matrix};

    _vertex_shader.set_uniform_block("transform_pack", matrices);
    _vertex_shader.bind(renderer->context());

    _pixel_shader.set_resource_view("diffuse_mtl", raw_ptr(_diffuse_mat));
    _pixel_shader.set_sampler("smpl", raw_ptr(_sampler));
    _pixel_shader.bind(renderer->context());
  }

  renderer->context()->RSSetState(raw_ptr(_rasterstate));
  renderer->context()->DrawIndexed(_index_count, 0, 0);
}

void app::simple_object::init(ID3D11Device* device) {
  _rasterstate = [device]() {
    CD3D11_RASTERIZER_DESC raster_state_desc{CD3D11_DEFAULT{}};
    raster_state_desc.FillMode = D3D11_FILL_WIREFRAME;
    raster_state_desc.CullMode = D3D11_CULL_NONE;

    com_ptr<ID3D11RasterizerState> new_state;
    device->CreateRasterizerState(&raster_state_desc, raw_ptr_ptr(new_state));

    return new_state;
  }();

  {
    constexpr auto  path = "c:/temp/assets/viper.obj";
    geometry_data_t mesh;
    geometry_factory::grid(16.0, 16.0f, 128, 128, &mesh);

    _index_count = mesh.index_count;

    ripple_params rp;
    rp.amplitude  = 0.6f;
    rp.period     = 3.0f * 6.2831853f;
    rp.grid_width = 16.0f;
    rp.grid_depth = 16.0f;

    ripple_grid(&mesh, rp);

    vector<vertex_pnt> vertices;
    vertices.reserve(mesh.vertex_count);
    transform(raw_ptr(mesh.geometry),
              raw_ptr(mesh.geometry) + mesh.vertex_count,
              back_inserter(vertices), [](const auto& in_vertex) {
                return vertex_pnt{in_vertex.position, in_vertex.normal,
                                  in_vertex.texcoords};
              });

    _vertex_buffer = [&vertices, device]() {
      D3D11_BUFFER_DESC buff_desc;
      buff_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
      buff_desc.ByteWidth =
          static_cast<uint32_t>(vertices.size() * sizeof(vertices[0]));
      buff_desc.CPUAccessFlags      = 0;
      buff_desc.MiscFlags           = 0;
      buff_desc.StructureByteStride = sizeof(vertices[0]);
      buff_desc.Usage               = D3D11_USAGE_IMMUTABLE;

      D3D11_SUBRESOURCE_DATA sdata;
      sdata.pSysMem     = &vertices[0];
      sdata.SysMemPitch = buff_desc.ByteWidth;

      com_ptr<ID3D11Buffer> vbuff;
      device->CreateBuffer(&buff_desc, &sdata, raw_ptr_ptr(vbuff));
      return vbuff;
    }();

    _index_buffer = [&mesh, device]() {
      D3D11_BUFFER_DESC buff_desc;
      buff_desc.BindFlags           = D3D11_BIND_INDEX_BUFFER;
      buff_desc.ByteWidth           = mesh.index_count * sizeof(uint32_t);
      buff_desc.CPUAccessFlags      = 0;
      buff_desc.MiscFlags           = 0;
      buff_desc.StructureByteStride = sizeof(uint32_t);
      buff_desc.Usage               = D3D11_USAGE_IMMUTABLE;

      D3D11_SUBRESOURCE_DATA sdata;
      sdata.pSysMem     = raw_ptr(mesh.indices);
      sdata.SysMemPitch = buff_desc.ByteWidth;

      com_ptr<ID3D11Buffer> ibuff;
      device->CreateBuffer(&buff_desc, &sdata, raw_ptr_ptr(ibuff));

      return ibuff;
    }();

    _vertex_shader = vertex_shader{device, "shaders/cap1/vert.hlsl", "main_pnt",
                                   shader_compile_flags::debug};
    if (!_vertex_shader)
      return;

    _pixel_shader = pixel_shader{device, "shaders/cap1/pixel.hlsl", "main_pnt",
                                 shader_compile_flags::debug};
    if (!_pixel_shader)
      return;

    _layout = [ signature = _vertex_shader.bytecode(), device ]() {
      const D3D11_INPUT_ELEMENT_DESC format_desc[] = {
          {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,
           D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
          {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,
           D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
          {"TEXCOORDS", 0, DXGI_FORMAT_R32G32_FLOAT, 0,
           D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0}};

      com_ptr<ID3D11InputLayout> layout;
      device->CreateInputLayout(format_desc, XR_U32_COUNTOF__(format_desc),
                                signature->GetBufferPointer(),
                                signature->GetBufferSize(),
                                raw_ptr_ptr(layout));

      return layout;
    }
    ();
  }

  {
    platformstl::memory_mapped_file tex_file{
        app_config::instance()->texture_path("uv_grids/ash_uvgrid02.jpg")};

    com_ptr<ID3D11Resource> loaded_tex;
    DirectX::CreateWICTextureFromMemoryEx(
        device, static_cast<const uint8_t*>(tex_file.memory()), tex_file.size(),
        0, D3D11_USAGE_IMMUTABLE, D3D11_BIND_SHADER_RESOURCE, 0, 0, true,
        raw_ptr_ptr(loaded_tex), nullptr);

    device->CreateShaderResourceView(raw_ptr(loaded_tex), nullptr,
                                     raw_ptr_ptr(_diffuse_mat));
    if (!_diffuse_mat) {
      XR_LOG_ERR("Failed to create shader resource view !!");
      return;
    }
  }

  {
    CD3D11_SAMPLER_DESC sampler_desc{CD3D11_DEFAULT{}};
    sampler_desc.Filter   = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sampler_desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    sampler_desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;

    device->CreateSamplerState(&sampler_desc, raw_ptr_ptr(_sampler));
    if (!_sampler) {
      XR_LOG_ERR("Failed to create sampler state!");
      return;
    }
  }

  _valid = true;
}
