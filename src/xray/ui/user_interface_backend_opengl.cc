#include "xray/ui/user_interface_backend_opengl.hpp"

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

#include "xray/base/logger.hpp"
#include "xray/math/objects/rectangle.hpp"
#include "xray/math/projection.hpp"
#include "xray/math/scalar4x4.hpp"
#include "xray/math/scalar4x4_math.hpp"
#include "xray/rendering/opengl/scoped_resource_mapping.hpp"
#include "xray/rendering/opengl/shader_base.hpp"
#include "xray/ui/user_interface_render_context.hpp"
#include <opengl/opengl.hpp>

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

tl::optional<xray::ui::UserInterfaceBackendOpengGL>
xray::ui::UserInterfaceBackendOpengGL::create(const UserInterfaceBackendCreateInfo& create_info)
{
    using namespace xray::rendering;

    scoped_buffer _vertex_buffer{ [&]() {
        GLuint vbuff{};
        gl::CreateBuffers(1, &vbuff);
        gl::NamedBufferStorage(vbuff, static_cast<GLsizeiptr>(create_info.max_vertices), nullptr, gl::MAP_WRITE_BIT);

        return vbuff;
    }() };

    if (_vertex_buffer) {
        XR_LOG_ERR("Failed to create vertex buffer!");
        return tl::nullopt;
    }

    scoped_buffer _index_buffer{ [&]() {
        GLuint ibuff{};
        gl::CreateBuffers(1, &ibuff);
        gl::NamedBufferStorage(ibuff, static_cast<GLsizeiptr>(create_info.max_indices), nullptr, gl::MAP_WRITE_BIT);
        return ibuff;
    }() };

    if (_index_buffer)
        return tl::nullopt;

    scoped_vertex_array _vertex_arr{
        [vbh = raw_handle(_vertex_buffer), ibh = raw_handle(_index_buffer), &create_info]() {
            GLuint vao{};
            gl::CreateVertexArrays(1, &vao);

            gl::VertexArrayVertexBuffer(vao, 0, vbh, 0, create_info.vertex_size);
            gl::VertexArrayElementBuffer(vao, ibh);

            gl::EnableVertexArrayAttrib(vao, 0);
            gl::EnableVertexArrayAttrib(vao, 1);
            gl::EnableVertexArrayAttrib(vao, 2);

            gl::VertexArrayAttribFormat(vao, 0, 2, gl::FLOAT, gl::FALSE_, create_info.offset0);
            gl::VertexArrayAttribFormat(vao, 1, 2, gl::FLOAT, gl::FALSE_, create_info.offset1);
            gl::VertexArrayAttribFormat(vao, 2, 4, gl::UNSIGNED_BYTE, gl::TRUE_, create_info.offset2);

            gl::VertexArrayAttribBinding(vao, 0, 0);
            gl::VertexArrayAttribBinding(vao, 1, 0);
            gl::VertexArrayAttribBinding(vao, 2, 0);

            return vao;
        }()
    };

    vertex_program _vs{ gpu_program_builder{}.add_string(IMGUI_VERTEX_SHADER).build<render_stage::e::vertex>() };

    if (!_vs) {
        XR_LOG_ERR("Failed to create vertex shader!");
        return tl::nullopt;
    }

    fragment_program _fs{ gpu_program_builder{}.add_string(IMGUI_FRAGMENT_SHADER).build<render_stage::e::fragment>() };

    if (!_fs) {
        XR_LOG_ERR("Failed to create vertex/fragment shaders!");
        return tl::nullopt;
    }

    program_pipeline _pipeline{ [&]() {
        GLuint phandle{};
        gl::CreateProgramPipelines(1, &phandle);
        return phandle;
    }() };

    _pipeline.use_vertex_program(_vs).use_fragment_program(_fs);

    scoped_texture _font_texture{ [&]() {
        GLuint texh{};

        gl::CreateTextures(gl::TEXTURE_2D, 1, &texh);
        gl::TextureStorage2D(texh, 1, gl::RGBA8, create_info.atlas_width, create_info.atlas_height);
        gl::TextureSubImage2D(texh,
                              0,
                              0,
                              0,
                              create_info.atlas_width,
                              create_info.atlas_height,
                              gl::RGBA,
                              gl::UNSIGNED_BYTE,
                              create_info.font_atlas_pixels.data());

        return texh;
    }() };

    scoped_sampler _font_sampler{ []() {
        GLuint smph{};
        gl::CreateSamplers(1, &smph);
        gl::SamplerParameteri(smph, gl::TEXTURE_MIN_FILTER, gl::LINEAR);
        gl::SamplerParameteri(smph, gl::TEXTURE_MAG_FILTER, gl::LINEAR);

        return smph;
    }() };

    return tl::make_optional<UserInterfaceBackendOpengGL>(PrivateConstructionToken{},
                                                          std::move(_vertex_buffer),
                                                          std::move(_index_buffer),
                                                          std::move(_vertex_arr),
                                                          std::move(_vs),
                                                          std::move(_fs),
                                                          std::move(_pipeline),
                                                          std::move(_font_texture),
                                                          std::move(_font_sampler),
                                                          create_info.max_vertices,
                                                          create_info.max_indices);
}

xray::ui::UserInterfaceBackendOpengGL::UserInterfaceBackendOpengGL(PrivateConstructionToken,
                                                                   xray::rendering::scoped_buffer vertex_buffer,
                                                                   xray::rendering::scoped_buffer index_buffer,
                                                                   xray::rendering::scoped_vertex_array vertex_arr,
                                                                   xray::rendering::vertex_program vs,
                                                                   xray::rendering::fragment_program fs,
                                                                   xray::rendering::program_pipeline pipeline,
                                                                   xray::rendering::scoped_texture font_texture,
                                                                   xray::rendering::scoped_sampler font_sampler,
                                                                   uint32_t vertex_buffer_size,
                                                                   uint32_t index_buffer_size)
    : _vertex_buffer{ std::move(vertex_buffer) }
    , _index_buffer{ std::move(index_buffer) }
    , _vertex_arr{ std::move(vertex_arr) }
    , _vs{ std::move(vs) }
    , _fs{ std::move(fs) }
    , _pipeline{ std::move(pipeline) }
    , _font_texture{ std::move(font_texture) }
    , _font_sampler{ std::move(font_sampler) }
    , _vertex_buffer_size{ vertex_buffer_size }
    , _index_buffer_size{ index_buffer_size }
{
}

void
xray::ui::UserInterfaceBackendOpengGL::render(const UserInterfaceRenderContext& draw_context)
{
    struct opengl_state_save_restore
    {
        GLint last_blend_src;
        GLint last_blend_dst;
        GLint last_blend_eq_rgb;
        GLint last_blend_eq_alpha;
        GLint last_viewport[4];
        GLint blend_enabled;
        GLint cullface_enabled;
        GLint depth_enabled;
        GLint scissors_enabled;

        opengl_state_save_restore()
        {
            gl::GetIntegerv(gl::BLEND_SRC, &last_blend_src);
            gl::GetIntegerv(gl::BLEND_DST, &last_blend_dst);
            gl::GetIntegerv(gl::BLEND_EQUATION_RGB, &last_blend_eq_rgb);
            gl::GetIntegerv(gl::BLEND_EQUATION_ALPHA, &last_blend_eq_alpha);
            gl::GetIntegerv(gl::VIEWPORT, last_viewport);
            blend_enabled = gl::IsEnabled(gl::BLEND);
            cullface_enabled = gl::IsEnabled(gl::CULL_FACE);
            depth_enabled = gl::IsEnabled(gl::DEPTH_TEST);
            scissors_enabled = gl::IsEnabled(gl::SCISSOR_TEST);
        }

        ~opengl_state_save_restore()
        {
            gl::BlendEquationSeparate(last_blend_eq_rgb, last_blend_eq_alpha);
            gl::BlendFunc(last_blend_src, last_blend_dst);
            blend_enabled ? gl::Enable(gl::BLEND) : gl::Disable(gl::BLEND);
            cullface_enabled ? gl::Enable(gl::CULL_FACE) : gl::Disable(gl::CULL_FACE);
            depth_enabled ? gl::Enable(gl::DEPTH_TEST) : gl::Disable(gl::DEPTH_TEST);
            scissors_enabled ? gl::Enable(gl::SCISSOR_TEST) : gl::Disable(gl::SCISSOR_TEST);
            gl::Viewport(last_viewport[0], last_viewport[1], last_viewport[2], last_viewport[3]);
        }
    } state_save_restore{};

    gl::Enable(gl::BLEND);
    gl::BlendEquation(gl::FUNC_ADD);
    gl::BlendFunc(gl::SRC_ALPHA, gl::ONE_MINUS_SRC_ALPHA);
    gl::Disable(gl::CULL_FACE);
    gl::Disable(gl::DEPTH_TEST);
    gl::Enable(gl::SCISSOR_TEST);

    const auto [draw_data, fb_width, fb_height] = draw_context;

    gl::Viewport(0, 0, fb_width, fb_height);

    using namespace xray::math;
    using namespace xray::rendering;

    const auto projection_mtx =
        orthographic(0.0f, static_cast<float>(fb_width), 0.0f, static_cast<float>(fb_height), -1.0f, +1.0f);

    _vs.set_uniform_block("matrix_pack", projection_mtx);
    _fs.set_uniform("font_texture", 0);
    _pipeline.use();
    gl::BindVertexArray(raw_handle(_vertex_arr));

    {
        const GLuint bound_samplers[] = { raw_handle(_font_sampler) };
        gl::BindSamplers(0, 1, bound_samplers);
    }

    for (int32_t lst_idx = 0; lst_idx < draw_data->CmdListsCount; ++lst_idx) {
        const auto cmd_lst = draw_data->CmdLists[lst_idx];
        const ImDrawIdx* idx_buff_offset = nullptr;

        {
            scoped_resource_mapping vb_map{ raw_handle(_vertex_buffer), gl::MAP_WRITE_BIT, _vertex_buffer_size };

            if (!vb_map)
                return;

            const auto vertex_bytes = cmd_lst->VtxBuffer.size() * sizeof(cmd_lst->VtxBuffer[0]);
            assert(vertex_bytes <= _vertex_buffer_size);
            memcpy(vb_map.memory(), &cmd_lst->VtxBuffer[0], vertex_bytes);
        }

        {
            scoped_resource_mapping ib_map{ raw_handle(_index_buffer), gl::MAP_WRITE_BIT, _index_buffer_size };

            if (!ib_map)
                return;

            const auto index_bytes = cmd_lst->IdxBuffer.size() * sizeof(cmd_lst->IdxBuffer[0]);
            assert(index_bytes <= _index_buffer_size);
            memcpy(ib_map.memory(), &cmd_lst->IdxBuffer[0], index_bytes);
        }

        for (auto cmd_itr = cmd_lst->CmdBuffer.begin(), cmd_end = cmd_lst->CmdBuffer.end(); cmd_itr != cmd_end;
             ++cmd_itr) {

            const GLuint textures_to_bind[] = { (GLuint)(intptr_t)cmd_itr->TextureId };
            gl::BindTextures(0, 1, textures_to_bind);

            gl::Scissor(static_cast<int32_t>(cmd_itr->ClipRect.x),
                        fb_height - static_cast<int32_t>(cmd_itr->ClipRect.w),
                        static_cast<int32_t>(cmd_itr->ClipRect.z - cmd_itr->ClipRect.x),
                        static_cast<int32_t>(cmd_itr->ClipRect.w - cmd_itr->ClipRect.y));

            gl::DrawElements(gl::TRIANGLES,
                             cmd_itr->ElemCount,
                             sizeof(ImDrawIdx) == 2 ? gl::UNSIGNED_SHORT : gl::UNSIGNED_INT,
                             idx_buff_offset);

            idx_buff_offset += cmd_itr->ElemCount;
        }
    }
}
