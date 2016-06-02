#include "simple_object.hpp"
#include "scoped_resource_mapping.hpp"
#include "xray/base/array_dimension.hpp"
#include "xray/math/constants.hpp"
#include "xray/math/scalar2x3.hpp"
#include "xray/math/scalar3.hpp"
#include "xray/math/scalar4.hpp"
#include "xray/math/transforms_r2.hpp"
#include "xray/rendering/vertex_format/vertex_pc.hpp"

using namespace xray::base;
using namespace xray::rendering;
using namespace xray::math;

static const vertex_pc Triangle_Vertices[] = {
    {{-0.8f, -0.8f, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
    {{0.8f, -0.8f, 0.0f}, {0.0f, 1.0f, 0.0f, 1.0f}},
    {{0.0f, 0.8f, 0.0f}, {0.0f, 0.0f, 1.0f, 1.0f}}};

static constexpr uint16_t Triangle_Indices[] = {0, 2, 1};

app::simple_object::simple_object(xray::rendering::directx10::renderer* r_ptr)
    : r_ptr_{r_ptr} {
  init(r_ptr);
}

app::simple_object::~simple_object() {}

void app::simple_object::init(xray::rendering::directx10::renderer* r_ptr) {
  assert(!valid());

  {
    vert_shader_ = vertex_shader{r_ptr, "shaders/vs_passthrough.hlsl"};
    if (!vert_shader_)
      return;

    frag_shader_ = fragment_shader{r_ptr, "shaders/fs_passthrough.hlsl"};
    if (!frag_shader_)
      return;
  }

  {
    vertex_buff_ =
        gpu_buffer{r_ptr, D3D10_BIND_VERTEX_BUFFER, Triangle_Vertices};

    if (!vertex_buff_)
      return;
  }

  {
    index_buff_ = gpu_buffer{r_ptr, D3D10_BIND_INDEX_BUFFER, Triangle_Indices};
    if (!index_buff_)
      return;
  }

  {
    // gpu_buffer(renderer* r_ptr, const size_t e_size, const size_t e_count,
    //        const uint32_t bind_flags, const void* initial_data = nullptr,
    //        const bool gpu_write = false, const bool cpu_acc = false);

    cbuffer_ = gpu_buffer{r_ptr,   32,    1,   D3D10_BIND_CONSTANT_BUFFER,
                          nullptr, false, true};
    if (!cbuffer_)
      return;
  }

  {
    constexpr D3D10_INPUT_ELEMENT_DESC VertexPC_Layout[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,
         D3D10_APPEND_ALIGNED_ELEMENT, D3D10_INPUT_PER_VERTEX_DATA, 0},
        {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0,
         D3D10_APPEND_ALIGNED_ELEMENT, D3D10_INPUT_PER_VERTEX_DATA, 0}};

    r_ptr->dev_ctx()->CreateInputLayout(
        VertexPC_Layout, XR_U32_COUNTOF__(VertexPC_Layout),
        vert_shader_.bytecode()->GetBufferPointer(),
        vert_shader_.bytecode()->GetBufferSize(), raw_ptr_ptr(layout_decl_));

    if (!layout_decl_)
      return;
  }

  {
    rasterizer_state front_ccw{};
    front_ccw.FrontCounterClockwise = true;
    r_ptr->dev_ctx()->CreateRasterizerState(&front_ccw, raw_ptr_ptr(rstate_));

    if (!rstate_)
      return;

    // r_ptr->dev_ctx()->RSSetState(raw_ptr(rstate_));
  }

  valid_ = true;
}

void app::simple_object::update(const float /* delta */) noexcept {
  angle_ = [](float ang_val) {
    if (ang_val > two_pi<float>)
      ang_val -= two_pi<float>;

    if (ang_val < two_pi<float>)
      ang_val += two_pi<float>;

    return ang_val;
  }(angle_ + 0.001f);
}

void app::simple_object::draw(const xray::rendering::draw_context_t&) {
  assert(valid());
  auto dev_ctx = r_ptr_->dev_ctx();

  {
    const uint32_t      strides[] = {(uint32_t) sizeof(Triangle_Vertices[0])};
    const uint32_t      offsets[] = {0};
    ID3D10Buffer* const buffers[] = {vertex_buff_.handle()};
    dev_ctx->IASetVertexBuffers(0, 1, buffers, strides, offsets);
    dev_ctx->IASetInputLayout(raw_ptr(layout_decl_));
    dev_ctx->IASetIndexBuffer(index_buff_.handle(), DXGI_FORMAT_R16_UINT, 0);
  }

  //
  // update constant buffers
  {
    const auto  rotation_matrix        = R2::rotate(angle_);
    const float rotation_matrix_data[] = {
        rotation_matrix.a00, rotation_matrix.a01, rotation_matrix.a02, 0.0f,

        rotation_matrix.a10, rotation_matrix.a11, rotation_matrix.a12, 0.0f};

    vert_shader_.set_uniform("rotation", rotation_matrix_data,
                             sizeof(rotation_matrix_data));
    vert_shader_.bind(r_ptr_);
    frag_shader_.bind(r_ptr_);
  }

  {
    dev_ctx->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    // dev_ctx->Draw(vertex_buff_.count(), 0);
    dev_ctx->DrawIndexed(index_buff_.count(), 0, 0);
  }
}
