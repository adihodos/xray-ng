#include "xray/ui/user_interface.hpp"
#include "xray/base/app_config.hpp"
#include "xray/base/logger.hpp"
#include "xray/base/pod_zero.hpp"
#include "xray/math/objects/rectangle.hpp"
#include "xray/math/projection.hpp"
#include "xray/math/scalar4x4.hpp"
#include "xray/math/scalar4x4_math.hpp"
#if defined(XRAY_RENDERER_DIRECTX)
#include "xray/rendering/directx/scoped_mapping.hpp"
#include "xray/rendering/directx/scoped_state.hpp"
#else
#include "xray/rendering/opengl/scoped_resource_mapping.hpp"
#include "xray/rendering/opengl/shader_base.hpp"
#include "xray/ui/user.interface.backend.hpp"
#include <opengl/opengl.hpp>
#endif
#include "xray/math/transforms_r4.hpp"
#include "xray/rendering/draw_context.hpp"
#include "xray/ui/events.hpp"
#include "xray/ui/key_sym.hpp"
#include "xray/ui/window.hpp"
#include <algorithm>

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

using namespace xray::base;
using namespace xray::rendering;
using namespace xray::ui;
using namespace xray::math;
using namespace std;

struct font_img_data
{
    uint8_t* pixels{};
    int32_t width{};
    int32_t height{};
    int32_t bpp{};
};

struct XrayKeyToImGuiKeyPair
{
    KeySymbol xr_keycode;
    ImGuiKey im_keycode;
};

static constexpr XrayKeyToImGuiKeyPair XR_KEY_TO_IMGUI_KEY_TABLE[126] = {
    { KeySymbol::ampersand, ImGuiKey_None },
    { KeySymbol::apostrophe, ImGuiKey_Apostrophe },
    { KeySymbol::asciicircum, ImGuiKey_None },
    { KeySymbol::asciitilde, ImGuiKey_None },
    { KeySymbol::asterisk, ImGuiKey_None },
    { KeySymbol::backslash, ImGuiKey_Backslash },
    { KeySymbol::backspace, ImGuiKey_Backspace },
    { KeySymbol::bar, ImGuiKey_None },
    { KeySymbol::braceleft, ImGuiKey_None },
    { KeySymbol::braceright, ImGuiKey_None },
    { KeySymbol::bracketleft, ImGuiKey_LeftBracket },
    { KeySymbol::bracketright, ImGuiKey_RightBracket },
    { KeySymbol::caps_lock, ImGuiKey_CapsLock },
    { KeySymbol::clear, ImGuiKey_None },
    { KeySymbol::comma, ImGuiKey_Comma },
    { KeySymbol::del, ImGuiKey_Delete },
    { KeySymbol::dollar, ImGuiKey_None },
    { KeySymbol::down, ImGuiKey_DownArrow },
    { KeySymbol::end, ImGuiKey_End },
    { KeySymbol::enter, ImGuiKey_Enter },
    { KeySymbol::escape, ImGuiKey_Escape },
    { KeySymbol::exclam, ImGuiKey_None },
    { KeySymbol::f1, ImGuiKey_F1 },
    { KeySymbol::f10, ImGuiKey_F10 },
    { KeySymbol::f11, ImGuiKey_F11 },
    { KeySymbol::f12, ImGuiKey_F12 },
    { KeySymbol::f13, ImGuiKey_F13 },
    { KeySymbol::f14, ImGuiKey_F14 },
    { KeySymbol::f15, ImGuiKey_F15 },
    { KeySymbol::f2, ImGuiKey_F2 },
    { KeySymbol::f3, ImGuiKey_F3 },
    { KeySymbol::f4, ImGuiKey_F4 },
    { KeySymbol::f5, ImGuiKey_F5 },
    { KeySymbol::f6, ImGuiKey_F6 },
    { KeySymbol::f7, ImGuiKey_F7 },
    { KeySymbol::f8, ImGuiKey_F8 },
    { KeySymbol::f9, ImGuiKey_F9 },
    { KeySymbol::grave, ImGuiKey_GraveAccent },
    { KeySymbol::home, ImGuiKey_Home },
    { KeySymbol::insert, ImGuiKey_Insert },
    { KeySymbol::key_0, ImGuiKey_0 },
    { KeySymbol::key_1, ImGuiKey_1 },
    { KeySymbol::key_2, ImGuiKey_2 },
    { KeySymbol::key_3, ImGuiKey_3 },
    { KeySymbol::key_4, ImGuiKey_4 },
    { KeySymbol::key_5, ImGuiKey_5 },
    { KeySymbol::key_6, ImGuiKey_6 },
    { KeySymbol::key_7, ImGuiKey_7 },
    { KeySymbol::key_8, ImGuiKey_8 },
    { KeySymbol::key_9, ImGuiKey_9 },
    { KeySymbol::key_a, ImGuiKey_A },
    { KeySymbol::key_b, ImGuiKey_B },
    { KeySymbol::key_c, ImGuiKey_C },
    { KeySymbol::key_d, ImGuiKey_D },
    { KeySymbol::key_e, ImGuiKey_E },
    { KeySymbol::key_f, ImGuiKey_F },
    { KeySymbol::key_g, ImGuiKey_G },
    { KeySymbol::key_h, ImGuiKey_H },
    { KeySymbol::key_i, ImGuiKey_I },
    { KeySymbol::key_j, ImGuiKey_J },
    { KeySymbol::key_k, ImGuiKey_K },
    { KeySymbol::key_l, ImGuiKey_L },
    { KeySymbol::key_m, ImGuiKey_M },
    { KeySymbol::key_n, ImGuiKey_N },
    { KeySymbol::key_o, ImGuiKey_O },
    { KeySymbol::key_p, ImGuiKey_P },
    { KeySymbol::key_q, ImGuiKey_Q },
    { KeySymbol::key_r, ImGuiKey_R },
    { KeySymbol::key_s, ImGuiKey_S },
    { KeySymbol::key_t, ImGuiKey_T },
    { KeySymbol::key_u, ImGuiKey_U },
    { KeySymbol::key_v, ImGuiKey_V },
    { KeySymbol::key_w, ImGuiKey_W },
    { KeySymbol::key_x, ImGuiKey_X },
    { KeySymbol::key_y, ImGuiKey_Y },
    { KeySymbol::key_z, ImGuiKey_Z },
    { KeySymbol::kp0, ImGuiKey_Keypad0 },
    { KeySymbol::kp1, ImGuiKey_Keypad1 },
    { KeySymbol::kp2, ImGuiKey_Keypad2 },
    { KeySymbol::kp3, ImGuiKey_Keypad3 },
    { KeySymbol::kp4, ImGuiKey_Keypad4 },
    { KeySymbol::kp5, ImGuiKey_Keypad5 },
    { KeySymbol::kp6, ImGuiKey_Keypad6 },
    { KeySymbol::kp7, ImGuiKey_Keypad7 },
    { KeySymbol::kp8, ImGuiKey_Keypad8 },
    { KeySymbol::kp9, ImGuiKey_Keypad9 },
    { KeySymbol::kp_add, ImGuiKey_KeypadAdd },
    { KeySymbol::kp_decimal, ImGuiKey_KeypadDecimal },
    { KeySymbol::kp_divide, ImGuiKey_KeypadDivide },
    { KeySymbol::kp_minus, ImGuiKey_KeypadSubtract },
    { KeySymbol::kp_multiply, ImGuiKey_KeypadMultiply },
    { KeySymbol::left, ImGuiKey_LeftArrow },
    { KeySymbol::left_alt, ImGuiKey_LeftAlt },
    { KeySymbol::left_control, ImGuiKey_LeftCtrl },
    { KeySymbol::left_menu, ImGuiKey_Menu },
    { KeySymbol::left_shift, ImGuiKey_LeftShift },
    { KeySymbol::left_win, ImGuiKey_LeftSuper },
    { KeySymbol::minus, ImGuiKey_Minus },
    { KeySymbol::num_lock, ImGuiKey_NumLock },
    { KeySymbol::numbersign, ImGuiKey_None },
    { KeySymbol::page_down, ImGuiKey_PageDown },
    { KeySymbol::page_up, ImGuiKey_PageUp },
    { KeySymbol::parenleft, ImGuiKey_None },
    { KeySymbol::parenright, ImGuiKey_None },
    { KeySymbol::pause, ImGuiKey_Pause },
    { KeySymbol::percent, ImGuiKey_None },
    { KeySymbol::period, ImGuiKey_Period },
    { KeySymbol::plus, ImGuiKey_None },
    { KeySymbol::print_screen, ImGuiKey_PrintScreen },
    { KeySymbol::quotedbl, ImGuiKey_None },
    { KeySymbol::quoteleft, ImGuiKey_None },
    { KeySymbol::quoteright, ImGuiKey_None },
    { KeySymbol::right, ImGuiKey_RightArrow },
    { KeySymbol::right_alt, ImGuiKey_RightAlt },
    { KeySymbol::right_control, ImGuiKey_RightCtrl },
    { KeySymbol::right_menu, ImGuiKey_Menu },
    { KeySymbol::right_shift, ImGuiKey_RightShift },
    { KeySymbol::right_win, ImGuiKey_RightSuper },
    { KeySymbol::scrol_lock, ImGuiKey_ScrollLock },
    { KeySymbol::select, ImGuiKey_None },
    { KeySymbol::slash, ImGuiKey_None },
    { KeySymbol::space, ImGuiKey_Space },
    { KeySymbol::tab, ImGuiKey_Tab },
    { KeySymbol::underscore, ImGuiKey_None },
    { KeySymbol::unknown, ImGuiKey_None },
    { KeySymbol::up, ImGuiKey_UpArrow },
};

static ImGuiKey
translate_key(const KeySymbol key_sym_val)
{
    assert(static_cast<size_t>(key_sym_val) < size(XR_KEY_TO_IMGUI_KEY_TABLE));
    return XR_KEY_TO_IMGUI_KEY_TABLE[static_cast<size_t>(key_sym_val)].im_keycode;
}

xray::ui::user_interface::user_interface() noexcept
{
    init(nullptr, 0);
}

xray::ui::user_interface::user_interface(const font_info* fonts, const size_t num_fonts)
{
    init(fonts, num_fonts);
}

#if defined(XRAY_RENDERER_OPENGL)
static constexpr const char* IMGUI_VERTEX_SHADER = "#version 450 core \n"
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
                                                   "out gl_PerVertex {\n"
                                                   "   vec4 gl_Position;\n"
                                                   "};\n"
                                                   "\n"
                                                   "void main() {\n"
                                                   "   gl_Position = projection * vec4(vs_in_pos, 0.0f, 1.0f);\n"
                                                   "   vs_out.frag_uv = vs_in_uv;\n"
                                                   "   vs_out.frag_col = vs_in_col;\n"
                                                   "}";

static constexpr const char* IMGUI_FRAGMENT_SHADER =
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
    "}";

#elif defined(XRAY_RENDERER_DIRECTX)

extern ID3D11Device* xr_render_device;
extern ID3D11DeviceContext* xr_render_device_context;

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

static constexpr auto IMGUI_SHADER_LEN = static_cast<uint32_t>(sizeof(IMGUI_SHADER));

#endif

void
xray::ui::user_interface::init(const font_info* fonts, const size_t num_fonts)
{
    IMGUI_CHECKVERSION();
    _imcontext = unique_pointer<ImGuiContext, imcontext_deleter>{ []() {
        auto imgui_ctx = ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;  // Enable Gamepad Controls
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;     // Enable Docking
        // io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;   // Enable Multi-Viewport / Platform Windows
        ImGui::StyleColorsDark();

        ImGuiStyle& style = ImGui::GetStyle();
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
            style.WindowRounding = 0.0f;
            style.Colors[ImGuiCol_WindowBg].w = 1.0f;
        }
        return imgui_ctx;
    }() };
    // _imcontext->IO.Fonts = new ImFontAtlas();

    set_current();

#if defined(XRAY_RENDERER_DIRECTX)

    _rendercontext.device = xr_render_device;
    _rendercontext.context = xr_render_device_context;

    {
        D3D11_BUFFER_DESC buffer_desc;
        buffer_desc.ByteWidth = user_interface::VERTEX_BUFFER_SIZE;
        buffer_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        buffer_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        buffer_desc.MiscFlags = 0;
        buffer_desc.StructureByteStride = 0;
        buffer_desc.Usage = D3D11_USAGE_DYNAMIC;

        _rendercontext.device->CreateBuffer(&buffer_desc, nullptr, raw_ptr_ptr(_rendercontext.vertex_buffer));
        if (!_rendercontext.vertex_buffer)
            return;
    }

    {
        D3D11_BUFFER_DESC buffer_desc;
        buffer_desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
        buffer_desc.ByteWidth = user_interface::INDEX_BUFFER_SIZE;
        buffer_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        buffer_desc.MiscFlags = 0;
        buffer_desc.StructureByteStride = 0;
        buffer_desc.Usage = D3D11_USAGE_DYNAMIC;

        _rendercontext.device->CreateBuffer(&buffer_desc, nullptr, raw_ptr_ptr(_rendercontext.index_buffer));
        if (!_rendercontext.index_buffer)
            return;
    }

    {
        load_fonts(fonts, num_fonts);
        font_img_data font_img;
        _gui->Fonts->GetTexDataAsRGBA32(&font_img.pixels, &font_img.width, &font_img.height, &font_img.bpp);
        if (!font_img.pixels)
            return;

        D3D11_TEXTURE2D_DESC tex_desc;
        tex_desc.ArraySize = 1;
        tex_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        tex_desc.CPUAccessFlags = 0;
        tex_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        tex_desc.Height = static_cast<uint32_t>(font_img.height);
        tex_desc.Width = static_cast<uint32_t>(font_img.width);
        tex_desc.MipLevels = 1;
        tex_desc.MiscFlags = 0;
        tex_desc.SampleDesc.Count = 1;
        tex_desc.SampleDesc.Quality = 0;
        tex_desc.Usage = D3D11_USAGE_IMMUTABLE;

        D3D11_SUBRESOURCE_DATA tex_data;
        tex_data.pSysMem = font_img.pixels;
        tex_data.SysMemPitch = font_img.width * 4;
        tex_data.SysMemSlicePitch = font_img.width * font_img.height * 4;

        com_ptr<ID3D11Texture2D> font_texture;
        _rendercontext.device->CreateTexture2D(&tex_desc, &tex_data, raw_ptr_ptr(font_texture));

        if (!font_texture)
            return;

        _rendercontext.device->CreateShaderResourceView(
            raw_ptr(font_texture), nullptr, raw_ptr_ptr(_rendercontext.font_texture));

        if (!_rendercontext.font_texture)
            return;

        _gui->Fonts->TexID = static_cast<void*>(raw_ptr(_rendercontext.font_texture));
    }

    {
        pod_zero<D3D11_SAMPLER_DESC> sampler_desc;
        sampler_desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        sampler_desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
        sampler_desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
        sampler_desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
        sampler_desc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;

        _rendercontext.device->CreateSamplerState(&sampler_desc, raw_ptr_ptr(_rendercontext.font_sampler));
        if (!_rendercontext.font_sampler)
            return;
    }

    {
        _rendercontext.vs = vertex_shader{ _rendercontext.device, IMGUI_SHADER, IMGUI_SHADER_LEN, "vs_main" };
        if (!_rendercontext.vs)
            return;

        _rendercontext.ps = pixel_shader{ _rendercontext.device, IMGUI_SHADER, IMGUI_SHADER_LEN, "ps_main" };
        if (!_rendercontext.ps)
            return;
    }

    {
        pod_zero<D3D11_BLEND_DESC> desc;
        desc.AlphaToCoverageEnable = false;
        desc.RenderTarget[0].BlendEnable = true;
        desc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
        desc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
        desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
        desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
        desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
        desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
        desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

        _rendercontext.device->CreateBlendState(&desc, raw_ptr_ptr(_rendercontext.blend_state));
        if (!_rendercontext.blend_state)
            return;
    }

    {
        CD3D11_DEPTH_STENCIL_DESC dss_desc{ CD3D11_DEFAULT{} };
        dss_desc.DepthEnable = false;
        dss_desc.StencilEnable = false;

        _rendercontext.device->CreateDepthStencilState(&dss_desc, raw_ptr_ptr(_rendercontext.depth_stencil_state));
        if (!_rendercontext.depth_stencil_state)
            return;
    }

    {
        pod_zero<D3D11_RASTERIZER_DESC> desc;
        desc.FillMode = D3D11_FILL_SOLID;
        desc.CullMode = D3D11_CULL_NONE;
        desc.ScissorEnable = true;
        desc.DepthClipEnable = true;

        _rendercontext.device->CreateRasterizerState(&desc, raw_ptr_ptr(_rendercontext.raster_state));

        if (!_rendercontext.raster_state)
            return;
    }

    {
        const D3D11_INPUT_ELEMENT_DESC vertex_fmt_desc[] = {
            { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "COLOR",
              0,
              DXGI_FORMAT_R8G8B8A8_UNORM,
              0,
              XR_U32_OFFSETOF(ImDrawVert, col),
              D3D11_INPUT_PER_VERTEX_DATA,
              0 },
            { "TEXCOORD",
              0,
              DXGI_FORMAT_R32G32_FLOAT,
              0,
              XR_U32_OFFSETOF(ImDrawVert, uv),
              D3D11_INPUT_PER_VERTEX_DATA,
              0 }
        };

        auto signature_bytecode = _rendercontext.vs.bytecode();

        _rendercontext.device->CreateInputLayout(vertex_fmt_desc,
                                                 3,
                                                 signature_bytecode->GetBufferPointer(),
                                                 signature_bytecode->GetBufferSize(),
                                                 raw_ptr_ptr(_rendercontext.input_layout));

        if (!_rendercontext.input_layout)
            return;
    }

#else

    load_fonts(fonts, num_fonts);
    // ImGuiIO& io = ImGui::GetIO();
    // font_img_data font_img;
    // io.Fonts->GetTexDataAsRGBA32(&font_img.pixels, &font_img.width, &font_img.height, &font_img.bpp);
    //
    // io.Fonts->TexID = reinterpret_cast<void*>(raw_handle(_rendercontext._font_texture));

#endif
}

void
xray::ui::user_interface::load_fonts(const font_info* fonts, const size_t num_fonts)
{
    //
    // Default fonts that always get loaded (Proggy and FontAwesome)
    ImGuiIO& io = ImGui::GetIO();
    auto default_font = io.Fonts->AddFontDefault();
    _rendercontext.fonts.push_back({ "Default", 13.0f, default_font });

    ImFontConfig config;
    config.MergeMode = true;
    static const ImWchar icon_ranges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };
    auto font_awesome = io.Fonts->AddFontFromFileTTF(
        ConfigSystem::instance()->font_path("fontawesome/fontawesome-webfont.ttf").generic_string().c_str(),
        13.0f,
        &config,
        icon_ranges);
    assert(font_awesome != nullptr);
    _rendercontext.fonts.push_back({ "fontawesome", 13.0f, font_awesome });

    for_each(fonts, fonts + num_fonts, [this](const font_info& fi) {
        ImFontConfig config;
        config.OversampleH = 3;
        config.OversampleV = 1;
        config.GlyphExtraSpacing.x = 1.0f;

        auto fnt = ImGui::GetIO().Fonts->AddFontFromFileTTF(
            fi.path.generic_string().c_str(), fi.pixel_size, &config, ImGui::GetIO().Fonts->GetGlyphRangesDefault());

        if (!fnt) {
            XR_LOG_INFO("Failed to load font {}", fi.path.string());
            return;
        }

        _rendercontext.fonts.emplace_back(fi.path.stem().string(), fi.pixel_size, fnt);
    });

    sort(begin(_rendercontext.fonts), end(_rendercontext.fonts), [](const loaded_font& f0, const loaded_font& f1) {
        if (f0.name < f1.name) {
            return true;
        }

        if (f0.name > f1.name) {
            return false;
        }

        return f0.pixel_size <= f1.pixel_size;
    });

    for_each(begin(_rendercontext.fonts), end(_rendercontext.fonts), [](const loaded_font& fi) {
        XR_LOG_INFO("added font {}, size {} ...", fi.name, fi.pixel_size);
    });

    io.Fonts->GetTexDataAsRGBA32(&_rendercontext.atlas_data, &_rendercontext.atlas_width, &_rendercontext.atlas_height);
}

xray::ui::user_interface::~user_interface() noexcept
{
    set_current();
    ImGui::Shutdown();
}

tl::optional<xray::ui::UserInterfaceRenderContext>
xray::ui::user_interface::draw()
{
    ImGui::Render();
    auto draw_data = ImGui::GetDrawData();

#if defined(XRAY_RENDERER_DIRECTX)
    auto dc = _rendercontext.context;

    {
        scoped_resource_mapping vb_map{ dc, raw_ptr(_rendercontext.vertex_buffer), D3D11_MAP_WRITE_DISCARD };
        if (!vb_map)
            return;

        scoped_resource_mapping ib_map{ dc, raw_ptr(_rendercontext.index_buffer), D3D11_MAP_WRITE_DISCARD };
        if (!ib_map)
            return;

        auto vertices = static_cast<ImDrawVert*>(vb_map.memory());
        auto indices = static_cast<ImDrawIdx*>(ib_map.memory());

        for (int32_t i = 0; i < draw_data->CmdListsCount; ++i) {
            const auto cmd_lst = draw_data->CmdLists[i];

            memcpy(vertices, &cmd_lst->VtxBuffer[0], cmd_lst->VtxBuffer.size() * sizeof(cmd_lst->VtxBuffer[0]));
            memcpy(indices, &cmd_lst->IdxBuffer[0], cmd_lst->IdxBuffer.size() * sizeof(cmd_lst->IdxBuffer[0]));

            vertices += cmd_lst->VtxBuffer.size();
            indices += cmd_lst->IdxBuffer.size();
        }
    }

    auto& gui = ImGui::GetIO();
    {
        const auto proj_matrix =
            projections_lh::orthographic(0.0f, gui.DisplaySize.x, 0.0f, gui.DisplaySize.y, 0.0f, 1.0f);

        _rendercontext.vs.set_uniform_block("matrix_pack", proj_matrix);
        _rendercontext.ps.set_sampler("font_sampler", raw_ptr(_rendercontext.font_sampler));
        _rendercontext.vs.bind(dc);
    }

    const float blend_factor[4] = { 0.f, 0.f, 0.f, 0.f };
    scoped_blend_state blend_state{ dc, raw_ptr(_rendercontext.blend_state), blend_factor, 0xffffffff };

    scoped_depth_stencil_state ds_state{ dc, raw_ptr(_rendercontext.depth_stencil_state), 0 };
    scoped_rasterizer_state raster_state{ dc, raw_ptr(_rendercontext.raster_state) };
    scoped_scissor_rects scissor_rects{ dc, nullptr, 0 };

    D3D11_VIEWPORT viewport;
    viewport.Width = gui.DisplaySize.x;
    viewport.Height = gui.DisplaySize.y;
    viewport.MaxDepth = 1.0f;
    viewport.MinDepth = 0.0f;
    viewport.TopLeftX = viewport.TopLeftY = 0.0f;

    scoped_viewport viewports{ dc, &viewport, 1 };

    {
        ID3D11Buffer* bound_vbs[] = { raw_ptr(_rendercontext.vertex_buffer) };
        const uint32_t strides = static_cast<uint32_t>(sizeof(ImDrawVert));
        const uint32_t offsets = 0;
        dc->IASetVertexBuffers(0, 1, bound_vbs, &strides, &offsets);
        dc->IASetIndexBuffer(raw_ptr(_rendercontext.index_buffer),
                             sizeof(ImDrawIdx) == 2 ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT,
                             0);
        dc->IASetInputLayout(raw_ptr(_rendercontext.input_layout));
        dc->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    }

    uint32_t vertex_offset{};
    uint32_t index_offset{};

    for (int32_t i = 0; i < draw_data->CmdListsCount; ++i) {
        const auto cmd_list = draw_data->CmdLists[i];

        for (int32_t n = 0; n < cmd_list->CmdBuffer.size(); ++n) {
            const auto cmd = &cmd_list->CmdBuffer[n];

            const D3D11_RECT scissor_rc{ static_cast<LONG>(cmd->ClipRect.x),
                                         static_cast<LONG>(cmd->ClipRect.y),
                                         static_cast<LONG>(cmd->ClipRect.z),
                                         static_cast<LONG>(cmd->ClipRect.w) };

            dc->RSSetScissorRects(1, &scissor_rc);
            _rendercontext.ps.set_resource_view("font_tex", static_cast<ID3D11ShaderResourceView*>(cmd->TextureId));
            _rendercontext.ps.bind(dc);
            dc->DrawIndexed(cmd->ElemCount, index_offset, vertex_offset);
            index_offset += cmd->ElemCount;
        }

        vertex_offset += cmd_list->VtxBuffer.size();
    }

#else
    ImGuiIO& io = ImGui::GetIO();
    const auto fb_width = static_cast<int32_t>(io.DisplaySize.x * io.DisplayFramebufferScale.x);
    const auto fb_height = static_cast<int32_t>(io.DisplaySize.y * io.DisplayFramebufferScale.y);
    if (fb_width == 0 || fb_height == 0)
        return tl::nullopt;

    const float scale[2] = { 2.0f / draw_data->DisplaySize.x, 2.0f / draw_data->DisplaySize.y };
    const float translate[2] = { -1.0f - draw_data->DisplayPos.x * scale[0],
                                 -1.0f - draw_data->DisplayPos.y * scale[1] };

    return UserInterfaceRenderContext{ .draw_data = draw_data,
                                       .fb_width = fb_width,
                                       .fb_height = fb_height,
                                       .scale_x = scale[0],
                                       .scale_y = scale[1],
                                       .translate_x = translate[0],
                                       .translate_y = translate[1],
                                       .textureid = _rendercontext.font_atlas_handle };
#endif
}

void
xray::ui::user_interface::new_frame(const int32_t wnd_width, const int32_t wnd_height)
{
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2{ static_cast<float>(wnd_width), static_cast<float>(wnd_height) };

    if (wnd_width > 0 && wnd_height > 0) {
        // io.DisplayFramebufferScale = ImVec2{};
    }

    ImGui::NewFrame();
}

bool
xray::ui::user_interface::input_event(const xray::ui::window_event& in_evt)
{
    if (in_evt.type == event_type::key) {
        const auto& ke = in_evt.event.key;
        const bool is_down{ ke.type == event_action_type::press };

        ImGuiIO& io = ImGui::GetIO();
        const ImGuiKey translated_key{ translate_key(ke.keycode) };
        if (translated_key != ImGuiKey_None)
            io.AddKeyEvent(translated_key, ke.type == event_action_type::press);

        if (ke.keycode == KeySymbol::left_control || ke.keycode == KeySymbol::right_control)
            io.AddKeyEvent(ImGuiMod_Ctrl, is_down);

        if (ke.keycode == KeySymbol::left_shift || ke.keycode == KeySymbol::right_shift)
            io.AddKeyEvent(ImGuiMod_Shift, is_down);

        if (ke.keycode == KeySymbol::left_alt || ke.keycode == KeySymbol::right_alt)
            io.AddKeyEvent(ImGuiMod_Alt, is_down);

        if (ke.keycode == KeySymbol::left_menu || ke.keycode == KeySymbol::right_win)
            io.AddKeyEvent(ImGuiMod_Super, is_down);

        return true;
    }

    if (in_evt.type == event_type::char_input) {
        const char_input_event* ch_in = &in_evt.event.char_input;
        ImGuiIO& io = ImGui::GetIO();
        if (ch_in->wnd->core.key_sym_handling == InputKeySymHandling::Unicode)
            io.AddInputCharacter(ch_in->unicode_point);
        else
            io.AddInputCharactersUTF8(ch_in->utf8);
        return true;
    }

    if (in_evt.type == event_type::mouse_button) {
        const auto& mb = in_evt.event.button;

        ImGuiIO& io = ImGui::GetIO();
        io.AddKeyEvent(ImGuiMod_Ctrl, mb.control);
        io.AddKeyEvent(ImGuiMod_Shift, mb.shift);

        bool handled = true;

        switch (mb.button) {
            case mouse_button::button1:
                io.MouseDown[0] = mb.type == event_action_type::press;
                break;

            case mouse_button::button3:
                io.MouseDown[1] = mb.type == event_action_type::press;
                break;

            default:
                handled = false;
                break;
        }

        return handled;
    }

    if (in_evt.type == event_type::mouse_motion) {
        const auto& mm = in_evt.event.motion;
        ImGuiIO& io = ImGui::GetIO();
        io.AddMousePosEvent(static_cast<float>(mm.pointer_x), static_cast<float>(mm.pointer_y));

        return true;
    }

    if (in_evt.type == event_type::mouse_wheel) {
        ImGuiIO& io = ImGui::GetIO();
        const auto& mw = in_evt.event.wheel;

        io.AddMouseWheelEvent(0.0f, static_cast<float>(mw.delta));
        return true;
    }

    return false;
}

void
xray::ui::user_interface::tick(const float delta)
{
    ImGui::GetIO().DeltaTime = delta / 1000.0f;
}

bool
xray::ui::user_interface::wants_input() const noexcept
{
    const ImGuiIO io = ImGui::GetIO();
    return io.WantCaptureMouse || io.WantCaptureKeyboard;
}

xray::ui::user_interface::loaded_font*
xray::ui::user_interface::find_font(const char* name)
{
    if (name == nullptr) {
        name = "Default";
    }

    auto fentry = find_if(begin(_rendercontext.fonts), end(_rendercontext.fonts), [name](const loaded_font& fi) {
        return fi.name == name;
    });

    if (fentry == end(_rendercontext.fonts)) {
        XR_LOG_INFO("Trying to set non existent font {}", name);
        return nullptr;
    }

    return &*fentry;
}

void
xray::ui::user_interface::set_global_font(const char* name)
{
    auto fnt_entry = find_font(name);
    if (fnt_entry) {
        ImGui::GetIO().FontDefault = fnt_entry->font;
    }
}

void
xray::ui::user_interface::push_font(const char* name)
{
    auto fnt_entry = find_font(name);
    if (fnt_entry) {
        ImGui::PushFont(fnt_entry->font);
    }
}

void
xray::ui::user_interface::pop_font()
{
    ImGui::PopFont();
}

xray::ui::UserInterfaceBackendCreateInfo
xray::ui::user_interface::render_backend_create_info() noexcept
{
    return UserInterfaceBackendCreateInfo{
        .upload_callback = &user_interface::font_atlas_upload_callback,
        .upload_cb_context = static_cast<void*>(this),
        .font_atlas_pixels =
            std::span<const uint8_t>{ _rendercontext.atlas_data,
                                      (size_t)(_rendercontext.atlas_width * _rendercontext.atlas_height * 4) },
        .atlas_width = (uint32_t)_rendercontext.atlas_width,
        .atlas_height = (uint32_t)_rendercontext.atlas_height,
    };
}
