#include "xray/ui/user_interface.hpp"
#include "xray/base/logger.hpp"
#include "xray/base/pod_zero.hpp"
#include "xray/math/projection.hpp"
#include "xray/math/rectangle.hpp"
#include "xray/math/scalar4x4.hpp"
#if defined(XRAY_RENDERER_DIRECTX)
#include "xray/rendering/directx/scoped_mapping.hpp"
#include "xray/rendering/directx/scoped_state.hpp"
#else
#include "xray/rendering/opengl/scoped_resource_mapping.hpp"
#include "xray/rendering/opengl/shader_base.hpp"
#include <opengl/opengl.hpp>
#endif
#include "xray/rendering/draw_context.hpp"
#include "xray/ui/input_event.hpp"
#include "xray/ui/key_symbols.hpp"
#include <imgui/imgui.h>

using namespace xray::base;
using namespace xray::rendering;
using namespace xray::ui;
using namespace xray::math;

struct font_img_data {
  uint8_t* pixels{};
  int32_t  width{};
  int32_t  height{};
  int32_t  bpp{};
};

#if defined(XRAY_RENDERER_OPENGL)
static constexpr const char* IMGUI_VERTEX_SHADER[] = {
    "#version 450 core \n"
    "\n"
    "layout (row_major) uniform; \n"
    "layout (location = 0) in vec2 vs_in_pos;\n"
    "layout (location = 1) in vec2 vs_in_uv;\n"
    "layout (location = 2) in vec4 vs_in_col;\n"
    "\n"
    "out PS_IN {\n"
    "   layout (location = 0) vec2 frag_uv;\n"
    "   layout (location = 1) vec4 frag_col;\n"
    "} vs_out;\n"
    "\n"
    "layout (binding = 0) uniform matrix_pack {\n"
    "   mat4 projection;\n"
    "};\n"
    "\n"
    "void main() {\n"
    "   gl_Position = projection * vec4(vs_in_pos, 0.0f, 1.0f);\n"
    "   vs_out.frag_uv = vs_in_uv;\n"
    "   vs_out.frag_col = vs_in_col;\n"
    "}"};

static constexpr const char* IMGUI_FRAGMENT_SHADER[] = {
    "#version 450 core \n"
    "\n"
    "in PS_IN {\n"
    "   layout (location = 0) vec2 frag_uv;\n"
    "   layout (location = 1) vec4 frag_col;\n"
    "} ps_in;\n"
    "\n"
    "layout (location = 0) out vec4 frag_color;\n"
    "\n"
    "uniform sampler2D font_texture;\n"
    "\n"
    "void main() {\n"
    "   frag_color = ps_in.frag_col * texture2D(font_texture, ps_in.frag_uv); "
    "\n"
    "}"};

#else
static constexpr const char IMGUI_SHADER[] = "#pragma pack_matrix(row_major)\n \
struct nk_vs_in { \
    float2 pos : POSITION; \
    float4 color : COLOR; \
    float2 uv : TEXCOORD; \
}; \
    \
struct nk_ps_in { \
    float4 pos : SV_POSITION; \
    float4 color : COLOR; \
    float2 uv : TEXCOORD; \
}; \
\
cbuffer matrix_pack { \
    float4x4 world_view_proj; \
}; \
\
\
nk_ps_in vs_main(in const nk_vs_in vs_in) { \
    nk_ps_in vs_out; \
    vs_out.pos = mul(world_view_proj, float4(vs_in.pos, 0.0f, 1.0f)); \
    vs_out.color = vs_in.color; \
    vs_out.uv = vs_in.uv; \
\
    return vs_out; \
};\
\
Texture2D font_tex; \
SamplerState font_sampler; \
float4 ps_main(in const nk_ps_in ps_in) : SV_TARGET { \
    return ps_in.color * font_tex.Sample(font_sampler, ps_in.uv); \
}";

static constexpr auto IMGUI_SHADER_LEN =
    static_cast<uint32_t>(sizeof(IMGUI_SHADER));

#endif

#if defined(XRAY_RENDERER_DIRECTX)
xray::ui::imgui_backend::imgui_backend(ID3D11Device*        device,
                                       ID3D11DeviceContext* context) noexcept
    : _gui{&ImGui::GetIO()} {
  _rcon.device  = device;
  _rcon.context = context;

  {
    D3D11_BUFFER_DESC buffer_desc;
    buffer_desc.ByteWidth           = imgui_backend::VERTEX_BUFFER_SIZE;
    buffer_desc.BindFlags           = D3D11_BIND_VERTEX_BUFFER;
    buffer_desc.CPUAccessFlags      = D3D11_CPU_ACCESS_WRITE;
    buffer_desc.MiscFlags           = 0;
    buffer_desc.StructureByteStride = 0;
    buffer_desc.Usage               = D3D11_USAGE_DYNAMIC;

    _rcon.device->CreateBuffer(&buffer_desc, nullptr,
                               raw_ptr_ptr(_rcon.vertex_buffer));
    if (!_rcon.vertex_buffer)
      return;
  }

  {
    D3D11_BUFFER_DESC buffer_desc;
    buffer_desc.BindFlags           = D3D11_BIND_INDEX_BUFFER;
    buffer_desc.ByteWidth           = imgui_backend::INDEX_BUFFER_SIZE;
    buffer_desc.CPUAccessFlags      = D3D11_CPU_ACCESS_WRITE;
    buffer_desc.MiscFlags           = 0;
    buffer_desc.StructureByteStride = 0;
    buffer_desc.Usage               = D3D11_USAGE_DYNAMIC;

    _rcon.device->CreateBuffer(&buffer_desc, nullptr,
                               raw_ptr_ptr(_rcon.index_buffer));
    if (!_rcon.index_buffer)
      return;
  }

  {
    font_img_data font_img;
    _gui->Fonts->GetTexDataAsRGBA32(&font_img.pixels, &font_img.width,
                                    &font_img.height, &font_img.bpp);
    if (!font_img.pixels)
      return;

    D3D11_TEXTURE2D_DESC tex_desc;
    tex_desc.ArraySize          = 1;
    tex_desc.BindFlags          = D3D11_BIND_SHADER_RESOURCE;
    tex_desc.CPUAccessFlags     = 0;
    tex_desc.Format             = DXGI_FORMAT_R8G8B8A8_UNORM;
    tex_desc.Height             = static_cast<uint32_t>(font_img.height);
    tex_desc.Width              = static_cast<uint32_t>(font_img.width);
    tex_desc.MipLevels          = 1;
    tex_desc.MiscFlags          = 0;
    tex_desc.SampleDesc.Count   = 1;
    tex_desc.SampleDesc.Quality = 0;
    tex_desc.Usage              = D3D11_USAGE_IMMUTABLE;

    D3D11_SUBRESOURCE_DATA tex_data;
    tex_data.pSysMem          = font_img.pixels;
    tex_data.SysMemPitch      = font_img.width * 4;
    tex_data.SysMemSlicePitch = font_img.width * font_img.height * 4;

    com_ptr<ID3D11Texture2D> font_texture;
    _rcon.device->CreateTexture2D(&tex_desc, &tex_data,
                                  raw_ptr_ptr(font_texture));

    if (!font_texture)
      return;

    _rcon.device->CreateShaderResourceView(raw_ptr(font_texture), nullptr,
                                           raw_ptr_ptr(_rcon.font_texture));

    if (!_rcon.font_texture)
      return;

    _gui->Fonts->TexID = static_cast<void*>(raw_ptr(_rcon.font_texture));
  }

  {
    pod_zero<D3D11_SAMPLER_DESC> sampler_desc;
    sampler_desc.Filter         = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sampler_desc.AddressU       = D3D11_TEXTURE_ADDRESS_WRAP;
    sampler_desc.AddressV       = D3D11_TEXTURE_ADDRESS_WRAP;
    sampler_desc.AddressW       = D3D11_TEXTURE_ADDRESS_WRAP;
    sampler_desc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;

    _rcon.device->CreateSamplerState(&sampler_desc,
                                     raw_ptr_ptr(_rcon.font_sampler));
    if (!_rcon.font_sampler)
      return;
  }

  {
    _rcon.vs =
        vertex_shader{_rcon.device, IMGUI_SHADER, IMGUI_SHADER_LEN, "vs_main"};
    if (!_rcon.vs)
      return;

    _rcon.ps =
        pixel_shader{_rcon.device, IMGUI_SHADER, IMGUI_SHADER_LEN, "ps_main"};
    if (!_rcon.ps)
      return;
  }

  {
    pod_zero<D3D11_BLEND_DESC> desc;
    desc.AlphaToCoverageEnable                 = false;
    desc.RenderTarget[0].BlendEnable           = true;
    desc.RenderTarget[0].SrcBlend              = D3D11_BLEND_SRC_ALPHA;
    desc.RenderTarget[0].DestBlend             = D3D11_BLEND_INV_SRC_ALPHA;
    desc.RenderTarget[0].BlendOp               = D3D11_BLEND_OP_ADD;
    desc.RenderTarget[0].SrcBlendAlpha         = D3D11_BLEND_INV_SRC_ALPHA;
    desc.RenderTarget[0].DestBlendAlpha        = D3D11_BLEND_ZERO;
    desc.RenderTarget[0].BlendOpAlpha          = D3D11_BLEND_OP_ADD;
    desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

    _rcon.device->CreateBlendState(&desc, raw_ptr_ptr(_rcon.blend_state));
    if (!_rcon.blend_state)
      return;
  }

  {
    CD3D11_DEPTH_STENCIL_DESC dss_desc{CD3D11_DEFAULT{}};
    dss_desc.DepthEnable   = false;
    dss_desc.StencilEnable = false;

    _rcon.device->CreateDepthStencilState(
        &dss_desc, raw_ptr_ptr(_rcon.depth_stencil_state));
    if (!_rcon.depth_stencil_state)
      return;
  }

  {
    pod_zero<D3D11_RASTERIZER_DESC> desc;
    desc.FillMode        = D3D11_FILL_SOLID;
    desc.CullMode        = D3D11_CULL_NONE;
    desc.ScissorEnable   = true;
    desc.DepthClipEnable = true;

    _rcon.device->CreateRasterizerState(&desc, raw_ptr_ptr(_rcon.raster_state));

    if (!_rcon.raster_state)
      return;
  }

  {
    const D3D11_INPUT_ELEMENT_DESC vertex_fmt_desc[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0,
         D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0,
         XR_U32_OFFSETOF(ImDrawVert, col), D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0,
         XR_U32_OFFSETOF(ImDrawVert, uv), D3D11_INPUT_PER_VERTEX_DATA, 0}};

    auto signature_bytecode = _rcon.vs.bytecode();

    _rcon.device->CreateInputLayout(
        vertex_fmt_desc, 3, signature_bytecode->GetBufferPointer(),
        signature_bytecode->GetBufferSize(), raw_ptr_ptr(_rcon.input_layout));

    if (!_rcon.input_layout)
      return;
  }

  setup_key_mappings();
  _rcon.valid = true;
}
#else

xray::ui::imgui_backend::imgui_backend() noexcept : _gui{&ImGui::GetIO()} {
  _rendercontext._vertex_buffer = []() {
    GLuint vbuff{};
    gl::CreateBuffers(1, &vbuff);
    gl::NamedBufferStorage(vbuff, imgui_backend::VERTEX_BUFFER_SIZE, nullptr,
                           gl::MAP_WRITE_BIT);

    return vbuff;
  }();

  if (!_rendercontext._vertex_buffer) {
    XR_LOG_ERR("Failed to create vertex buffer!");
    return;
  }

  _rendercontext._index_buffer = []() {
    GLuint ibuff{};
    gl::CreateBuffers(1, &ibuff);
    gl::NamedBufferStorage(ibuff, imgui_backend::INDEX_BUFFER_SIZE, nullptr,
                           gl::MAP_WRITE_BIT);
    return ibuff;
  }();

  if (!_rendercontext._index_buffer)
    return;

  _rendercontext._vertex_arr = [
    vbh = raw_handle(_rendercontext._vertex_buffer),
    ibh = raw_handle(_rendercontext._index_buffer)
  ]() {
    GLuint vao{};
    gl::CreateVertexArrays(1, &vao);
    gl::BindVertexArray(vao);
    gl::BindBuffer(gl::ELEMENT_ARRAY_BUFFER, ibh);
    gl::VertexArrayVertexBuffer(vao, 0, vbh, 0, sizeof(ImDrawVert));

    gl::EnableVertexArrayAttrib(vao, 0);
    gl::EnableVertexArrayAttrib(vao, 1);
    gl::EnableVertexArrayAttrib(vao, 2);

    gl::VertexArrayAttribFormat(vao, 0, 2, gl::FLOAT, gl::FALSE_,
                                XR_U32_OFFSETOF(ImDrawVert, pos));
    gl::VertexArrayAttribFormat(vao, 1, 2, gl::FLOAT, gl::FALSE_,
                                XR_U32_OFFSETOF(ImDrawVert, uv));
    gl::VertexArrayAttribFormat(vao, 2, 4, gl::UNSIGNED_BYTE, gl::TRUE_,
                                XR_U32_OFFSETOF(ImDrawVert, col));

    gl::VertexArrayAttribBinding(vao, 0, 0);
    gl::VertexArrayAttribBinding(vao, 1, 0);
    gl::VertexArrayAttribBinding(vao, 2, 0);

    gl::BindVertexArray(0);
    gl::BindBuffer(gl::ELEMENT_ARRAY_BUFFER, 0);

    return vao;
  }
  ();

  _rendercontext._draw_prog = []() {
    const GLuint compiled_shaders[] = {
        make_shader(gl::VERTEX_SHADER, IMGUI_VERTEX_SHADER,
                    XR_U32_COUNTOF__(IMGUI_VERTEX_SHADER)),
        make_shader(gl::FRAGMENT_SHADER, IMGUI_FRAGMENT_SHADER,
                    XR_U32_COUNTOF__(IMGUI_FRAGMENT_SHADER))};

    return gpu_program{compiled_shaders};
  }();

  if (!_rendercontext._draw_prog) {
    XR_LOG_ERR("Failed to compile/link program!");
    return;
  }

  _rendercontext._font_texture = [g = _gui]() {
    GLuint texh{};

    font_img_data font_img;
    g->Fonts->GetTexDataAsRGBA32(&font_img.pixels, &font_img.width,
                                 &font_img.height, &font_img.bpp);

    gl::CreateTextures(gl::TEXTURE_2D, 1, &texh);
    gl::TextureStorage2D(texh, 1, gl::RGBA8, font_img.width, font_img.height);
    gl::TextureSubImage2D(texh, 0, 0, 0, font_img.width, font_img.height,
                          gl::RGBA, gl::UNSIGNED_BYTE, font_img.pixels);

    return texh;
  }
  ();

  _gui->Fonts->TexID =
      reinterpret_cast<void*>(raw_handle(_rendercontext._font_texture));

  _rendercontext._font_sampler = []() {
    GLuint smph{};
    gl::CreateSamplers(1, &smph);
    gl::SamplerParameteri(smph, gl::TEXTURE_MIN_FILTER, gl::LINEAR);
    gl::SamplerParameteri(smph, gl::TEXTURE_MAG_FILTER, gl::LINEAR);

    return smph;
  }();

  setup_key_mappings();
  _rendercontext._valid = true;
}

#endif

xray::ui::imgui_backend::~imgui_backend() noexcept { ImGui::Shutdown(); }

void xray::ui::imgui_backend::draw_event(
    const xray::rendering::draw_context_t& drw_ctx) {

  XR_UNUSED_ARG(drw_ctx);
  ImGui::Render();
  auto draw_data = ImGui::GetDrawData();

#if defined(XRAY_RENDERER_DIRECTX)
  auto dc = _rcon.context;

  {
    scoped_resource_mapping vb_map{dc, raw_ptr(_rcon.vertex_buffer),
                                   D3D11_MAP_WRITE_DISCARD};
    if (!vb_map)
      return;

    scoped_resource_mapping ib_map{dc, raw_ptr(_rcon.index_buffer),
                                   D3D11_MAP_WRITE_DISCARD};
    if (!ib_map)
      return;

    auto vertices = static_cast<ImDrawVert*>(vb_map.memory());
    auto indices  = static_cast<ImDrawIdx*>(ib_map.memory());

    for (int32_t i = 0; i < draw_data->CmdListsCount; ++i) {
      const auto cmd_lst = draw_data->CmdLists[i];

      memcpy(vertices, &cmd_lst->VtxBuffer[0],
             cmd_lst->VtxBuffer.size() * sizeof(cmd_lst->VtxBuffer[0]));
      memcpy(indices, &cmd_lst->IdxBuffer[0],
             cmd_lst->IdxBuffer.size() * sizeof(cmd_lst->IdxBuffer[0]));

      vertices += cmd_lst->VtxBuffer.size();
      indices += cmd_lst->IdxBuffer.size();
    }
  }

  auto& gui = ImGui::GetIO();
  {
    const auto proj_matrix = projection::orthographic(
        0.0f, gui.DisplaySize.x, 0.0f, gui.DisplaySize.y, 0.0f, 1.0f);

    _rcon.vs.set_uniform_block("matrix_pack", proj_matrix);
    _rcon.ps.set_sampler("font_sampler", raw_ptr(_rcon.font_sampler));
    _rcon.vs.bind(dc);
  }

  const float        blend_factor[4] = {0.f, 0.f, 0.f, 0.f};
  scoped_blend_state blend_state{dc, raw_ptr(_rcon.blend_state), blend_factor,
                                 0xffffffff};

  scoped_depth_stencil_state ds_state{dc, raw_ptr(_rcon.depth_stencil_state),
                                      0};
  scoped_rasterizer_state raster_state{dc, raw_ptr(_rcon.raster_state)};
  scoped_scissor_rects    scissor_rects{dc, nullptr, 0};

  D3D11_VIEWPORT viewport;
  viewport.Width    = gui.DisplaySize.x;
  viewport.Height   = gui.DisplaySize.y;
  viewport.MaxDepth = 1.0f;
  viewport.MinDepth = 0.0f;
  viewport.TopLeftX = viewport.TopLeftY = 0.0f;

  scoped_viewport viewports{dc, &viewport, 1};

  {
    ID3D11Buffer*  bound_vbs[] = {raw_ptr(_rcon.vertex_buffer)};
    const uint32_t strides     = static_cast<uint32_t>(sizeof(ImDrawVert));
    const uint32_t offsets     = 0;
    dc->IASetVertexBuffers(0, 1, bound_vbs, &strides, &offsets);
    dc->IASetIndexBuffer(raw_ptr(_rcon.index_buffer),
                         sizeof(ImDrawIdx) == 2 ? DXGI_FORMAT_R16_UINT
                                                : DXGI_FORMAT_R32_UINT,
                         0);
    dc->IASetInputLayout(raw_ptr(_rcon.input_layout));
    dc->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  }

  uint32_t vertex_offset{};
  uint32_t index_offset{};

  for (int32_t i = 0; i < draw_data->CmdListsCount; ++i) {
    const auto cmd_list = draw_data->CmdLists[i];

    for (int32_t n = 0; n < cmd_list->CmdBuffer.size(); ++n) {
      const auto cmd = &cmd_list->CmdBuffer[n];

      const D3D11_RECT scissor_rc{static_cast<LONG>(cmd->ClipRect.x),
                                  static_cast<LONG>(cmd->ClipRect.y),
                                  static_cast<LONG>(cmd->ClipRect.z),
                                  static_cast<LONG>(cmd->ClipRect.w)};

      dc->RSSetScissorRects(1, &scissor_rc);
      _rcon.ps.set_resource_view(
          "font_tex", static_cast<ID3D11ShaderResourceView*>(cmd->TextureId));
      _rcon.ps.bind(dc);
      dc->DrawIndexed(cmd->ElemCount, index_offset, vertex_offset);
      index_offset += cmd->ElemCount;
    }

    vertex_offset += cmd_list->VtxBuffer.size();
  }
#else
  const auto fb_width = static_cast<int32_t>(_gui->DisplaySize.x *
                                             _gui->DisplayFramebufferScale.x);
  const auto fb_height = static_cast<int32_t>(_gui->DisplaySize.y *
                                              _gui->DisplayFramebufferScale.y);
  if (fb_width == 0 || fb_height == 0)
    return;

  draw_data->ScaleClipRects(_gui->DisplayFramebufferScale);

  struct opengl_state_save_restore {
    GLint last_blend_src;
    GLint last_blend_dst;
    GLint last_blend_eq_rgb;
    GLint last_blend_eq_alpha;
    GLint last_viewport[4];
    GLint blend_enabled;
    GLint cullface_enabled;
    GLint depth_enabled;
    GLint scissors_enabled;

    opengl_state_save_restore() {
      gl::GetIntegerv(gl::BLEND_SRC, &last_blend_src);
      gl::GetIntegerv(gl::BLEND_DST, &last_blend_dst);
      gl::GetIntegerv(gl::BLEND_EQUATION_RGB, &last_blend_eq_rgb);
      gl::GetIntegerv(gl::BLEND_EQUATION_ALPHA, &last_blend_eq_alpha);
      gl::GetIntegerv(gl::VIEWPORT, last_viewport);
      blend_enabled    = gl::IsEnabled(gl::BLEND);
      cullface_enabled = gl::IsEnabled(gl::CULL_FACE);
      depth_enabled    = gl::IsEnabled(gl::DEPTH_TEST);
      scissors_enabled = gl::IsEnabled(gl::SCISSOR_TEST);
    }

    ~opengl_state_save_restore() {
      gl::BlendEquationSeparate(last_blend_eq_rgb, last_blend_eq_alpha);
      gl::BlendFunc(last_blend_src, last_blend_dst);
      blend_enabled ? gl::Enable(gl::BLEND) : gl::Disable(gl::BLEND);
      cullface_enabled ? gl::Enable(gl::CULL_FACE) : gl::Disable(gl::CULL_FACE);
      depth_enabled ? gl::Enable(gl::DEPTH_TEST) : gl::Disable(gl::DEPTH_TEST);
      scissors_enabled ? gl::Enable(gl::SCISSOR_TEST)
                       : gl::Disable(gl::SCISSOR_TEST);
      gl::Viewport(last_viewport[0], last_viewport[1], last_viewport[2],
                   last_viewport[3]);
    }
  } state_save_restore{};

  gl::Enable(gl::BLEND);
  gl::BlendEquation(gl::FUNC_ADD);
  gl::BlendFunc(gl::SRC_ALPHA, gl::ONE_MINUS_SRC_ALPHA);
  gl::Disable(gl::CULL_FACE);
  gl::Disable(gl::DEPTH_TEST);
  gl::Enable(gl::SCISSOR_TEST);

  gl::Viewport(0, 0, fb_width, fb_height);
  const auto projection_mtx =
      projection::ortho_off_center(0.0f, static_cast<float>(fb_width), 0.0f,
                                   static_cast<float>(fb_height), -1.0f, +1.0f);

  gl::UseProgram(_rendercontext._draw_prog.handle());
  _rendercontext._draw_prog.set_uniform_block("matrix_pack", projection_mtx);
  _rendercontext._draw_prog.set_uniform("font_texture", 0);
  _rendercontext._draw_prog.bind_to_pipeline();
  gl::BindVertexArray(raw_handle(_rendercontext._vertex_arr));

  {
    const GLuint bound_samplers[] = {raw_handle(_rendercontext._font_sampler)};
    gl::BindSamplers(0, 1, bound_samplers);
  }

  for (int32_t lst_idx = 0; lst_idx < draw_data->CmdListsCount; ++lst_idx) {
    const auto       cmd_lst         = draw_data->CmdLists[lst_idx];
    const ImDrawIdx* idx_buff_offset = nullptr;

    {
      scoped_resource_mapping vb_map{raw_handle(_rendercontext._vertex_buffer),
                                     gl::MAP_WRITE_BIT,
                                     imgui_backend::VERTEX_BUFFER_SIZE};

      if (!vb_map)
        return;

      const auto vertex_bytes =
          cmd_lst->VtxBuffer.size() * sizeof(cmd_lst->VtxBuffer[0]);
      assert(vertex_bytes <= imgui_backend::VERTEX_BUFFER_SIZE);
      memcpy(vb_map.memory(), &cmd_lst->VtxBuffer[0], vertex_bytes);
    }

    {
      scoped_resource_mapping ib_map{raw_handle(_rendercontext._index_buffer),
                                     gl::MAP_WRITE_BIT,
                                     imgui_backend::INDEX_BUFFER_SIZE};

      if (!ib_map)
        return;

      const auto index_bytes =
          cmd_lst->IdxBuffer.size() * sizeof(cmd_lst->IdxBuffer[0]);
      assert(index_bytes <= imgui_backend::INDEX_BUFFER_SIZE);
      memcpy(ib_map.memory(), &cmd_lst->IdxBuffer[0], index_bytes);
    }

    for (auto cmd_itr = cmd_lst->CmdBuffer.begin(),
              cmd_end = cmd_lst->CmdBuffer.end();
         cmd_itr != cmd_end; ++cmd_itr) {

      const GLuint textures_to_bind[] = {(GLuint)(intptr_t) cmd_itr->TextureId};
      gl::BindTextures(0, 1, textures_to_bind);

      gl::Scissor(
          static_cast<int32_t>(cmd_itr->ClipRect.x),
          fb_height - static_cast<int32_t>(cmd_itr->ClipRect.w),
          static_cast<int32_t>(cmd_itr->ClipRect.z - cmd_itr->ClipRect.x),
          static_cast<int32_t>(cmd_itr->ClipRect.w - cmd_itr->ClipRect.y));

      gl::DrawElements(gl::TRIANGLES, cmd_itr->ElemCount,
                       sizeof(ImDrawIdx) == 2 ? gl::UNSIGNED_SHORT
                                              : gl::UNSIGNED_INT,
                       idx_buff_offset);

      idx_buff_offset += cmd_itr->ElemCount;
    }
  }

#endif
}

void xray::ui::imgui_backend::new_frame(
    const xray::rendering::draw_context_t& dc) {

  _gui->DisplaySize = ImVec2{static_cast<float>(dc.window_width),
                             static_cast<float>(dc.window_height)};
  //  SetCursor(gui.MouseDrawCursor ? nullptr : LoadCursor(nullptr, IDC_ARROW));
  ImGui::NewFrame();
}

bool xray::ui::imgui_backend::input_event(
    const xray::ui::input_event_t& in_evt) {

  if (in_evt.type == input_event_type::key) {
    const auto& ke              = in_evt.key_ev;
    _gui->KeysDown[ke.key_code] = ke.down;
    return true;
  }

  if (in_evt.type == input_event_type::mouse_button) {
    const auto& mb      = in_evt.mouse_button_ev;
    bool        handled = true;

    switch (mb.btn_id) {
    case mouse_button::left:
      _gui->MouseDown[0] = mb.down;
      break;

    case mouse_button::right:
      _gui->MouseDown[1] = mb.down;
      break;

    default:
      handled = false;
      break;
    }

    return handled;
  }

  if (in_evt.type == input_event_type::mouse_movement) {
    const auto& mm   = in_evt.mouse_move_ev;
    _gui->MousePos.x = static_cast<float>(mm.x_pos);
    _gui->MousePos.y = static_cast<float>(mm.y_pos);

    return true;
  }

  if (in_evt.type == input_event_type::mouse_wheel) {
    const auto& mw = in_evt.mouse_wheel_ev;
    _gui->MouseWheel += mw.delta > 0 ? +1.0f : -1.0f;
    return true;
  }

  return false;
}

void xray::ui::imgui_backend::tick(const float delta) {
  ImGui::GetIO().DeltaTime = delta / 1000.0f;
}

bool xray::ui::imgui_backend::wants_input() const noexcept {
  return _gui->WantCaptureKeyboard || _gui->WantCaptureMouse;
}

void xray::ui::imgui_backend::setup_key_mappings() {
  _gui->KeyMap[ImGuiKey_Tab]        = key_symbol::tab;
  _gui->KeyMap[ImGuiKey_LeftArrow]  = key_symbol::left;
  _gui->KeyMap[ImGuiKey_RightArrow] = key_symbol::right;
  _gui->KeyMap[ImGuiKey_UpArrow]    = key_symbol::up;
  _gui->KeyMap[ImGuiKey_DownArrow]  = key_symbol::down;
  _gui->KeyMap[ImGuiKey_PageUp]     = key_symbol::page_up;
  _gui->KeyMap[ImGuiKey_PageDown]   = key_symbol::page_down;
  _gui->KeyMap[ImGuiKey_Home]       = key_symbol::home;
  _gui->KeyMap[ImGuiKey_End]        = key_symbol::end;
  _gui->KeyMap[ImGuiKey_Delete]     = key_symbol::del;
  _gui->KeyMap[ImGuiKey_Backspace]  = key_symbol::backspace;
  _gui->KeyMap[ImGuiKey_Enter]      = key_symbol::enter;
  _gui->KeyMap[ImGuiKey_Escape]     = key_symbol::escape;
  _gui->KeyMap[ImGuiKey_A]          = 'A';
  _gui->KeyMap[ImGuiKey_C]          = 'C';
  _gui->KeyMap[ImGuiKey_V]          = 'V';
  _gui->KeyMap[ImGuiKey_X]          = 'X';
  _gui->KeyMap[ImGuiKey_Y]          = 'Y';
  _gui->KeyMap[ImGuiKey_Z]          = 'Z';
  _gui->RenderDrawListsFn           = nullptr;
}
