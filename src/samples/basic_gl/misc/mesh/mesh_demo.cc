#include "misc/mesh/mesh_demo.hpp"
#include "init_context.hpp"
#include "xray/base/app_config.hpp"
#include "xray/base/array_dimension.hpp"
#include "xray/base/basic_timer.hpp"
#include "xray/math/constants.hpp"
#include "xray/math/projection.hpp"
#include "xray/math/scalar2.hpp"
#include "xray/math/scalar3_string_cast.hpp"
#include "xray/math/scalar4.hpp"
#include "xray/math/scalar4x4.hpp"
#include "xray/math/scalar4x4_math.hpp"
#include "xray/math/transforms_r3.hpp"
#include "xray/math/transforms_r4.hpp"
#include "xray/rendering/colors/color_palettes.hpp"
#include "xray/rendering/draw_context.hpp"
#include "xray/rendering/geometry/geometry_data.hpp"
#include "xray/rendering/geometry/geometry_factory.hpp"
#include "xray/rendering/mesh_loader.hpp"
#include "xray/rendering/opengl/scoped_opengl_setting.hpp"
#include "xray/rendering/opengl/scoped_resource_mapping.hpp"
#include "xray/rendering/texture_loader.hpp"
#include "xray/ui/events.hpp"
#include "xray/ui/user_interface.hpp"
#include <algorithm>
#include <cassert>
#include <cstring>
#include <iterator>
#include <opengl/opengl.hpp>
#include <span>
#include <vector>

using namespace xray::base;
using namespace xray::rendering;
using namespace xray::math;
using namespace xray::ui;
using namespace std;

extern xray::base::ConfigSystem* xr_app_config;

struct mesh_load_info
{
    const char* display_name;
    const char* file_name;
} const loadable_meshes[] = { { "A10 Thunderbolt II", "a10/a10.bin" },
                              { "A4", "a4/a4.bin" },
                              { "F15C Eagle", "f15/f15c.bin" },
                              { "F4 Phantom II", "f4/f4phantom.bin" },
                              { "Leopard I", "leo1/leo1.bin" },
                              { "Leopard 2A6", "leo2/leo2a6.bin" },
                              { "P38 Lighting", "p38/p38.bin" },
                              { "SA 23 Starfury Aurora", "sa23/sa23_aurora.bin" },
                              { "Eurofighter Typhoon", "typhoon/typhoon.bin" }

};

app::mesh_demo::mesh_demo(const init_context_t& init_ctx)
    : demo_base{ init_ctx }
{
    _camera.set_projection(
        perspective_symmetric(static_cast<float>(init_ctx.surface_width) / static_cast<float>(init_ctx.surface_height),
                              radians(70.0f),
                              0.1f,
                              100.0f));

    init();
    _ui->set_global_font("UbuntuMono-Regular");
}

app::mesh_demo::~mesh_demo() {}

void
app::mesh_demo::init()
{
    assert(!valid());

    //
    // turn off these so we don't get spammed
    gl::DebugMessageControl(gl::DONT_CARE, gl::DONT_CARE, gl::DEBUG_SEVERITY_NOTIFICATION, 0, nullptr, gl::FALSE_);

    if (!_abbdraw)
        return;

    gl::CreateBuffers(1, raw_handle_ptr(_vb));
    gl::CreateBuffers(1, raw_handle_ptr(_ib));

    {
        gl::CreateVertexArrays(1, raw_handle_ptr(_vao));
        gl::VertexArrayVertexBuffer(raw_handle(_vao), 0, raw_handle(_vb), 0, sizeof(vertex_pnt));
        gl::VertexArrayElementBuffer(raw_handle(_vao), raw_handle(_ib));

        gl::VertexArrayAttribFormat(raw_handle(_vao), 0, 3, gl::FLOAT, gl::FALSE_, offsetof(vertex_pnt, position));
        gl::VertexArrayAttribFormat(raw_handle(_vao), 1, 3, gl::FLOAT, gl::FALSE_, offsetof(vertex_pnt, normal));
        gl::VertexArrayAttribFormat(raw_handle(_vao), 2, 2, gl::FLOAT, gl::FALSE_, offsetof(vertex_pnt, texcoord));

        gl::VertexArrayAttribBinding(raw_handle(_vao), 0, 0);
        gl::VertexArrayAttribBinding(raw_handle(_vao), 1, 0);
        gl::VertexArrayAttribBinding(raw_handle(_vao), 2, 0);

        gl::EnableVertexArrayAttrib(raw_handle(_vao), 0);
        gl::EnableVertexArrayAttrib(raw_handle(_vao), 1);
        gl::EnableVertexArrayAttrib(raw_handle(_vao), 2);
    }

    _vs = gpu_program_builder{}.add_file("shaders/misc/mesh/vs.glsl").build<render_stage::e::vertex>();

    if (!_vs)
        return;

    _fs = gpu_program_builder{}.add_file("shaders/misc/mesh/fs.glsl").build<render_stage::e::fragment>();

    if (!_fs)
        return;

    _vsnormals = gpu_program_builder{}.add_file("shaders/misc/mesh/vs.pass.glsl").build<render_stage::e::vertex>();

    if (!_vsnormals)
        return;

    _gsnormals = gpu_program_builder{}.add_file("shaders/misc/mesh/gs.glsl").build<render_stage::e::geometry>();

    if (!_gsnormals)
        return;

    _fsnormals = gpu_program_builder{}.add_file("shaders/misc/mesh/fs.color.glsl").build<render_stage::e::fragment>();

    if (!_fsnormals)
        return;

    _pipeline = program_pipeline{ []() {
        GLuint ppl{};
        gl::CreateProgramPipelines(1, &ppl);
        return ppl;
    }() };

    texture_loader tldr{ xr_app_config->texture_path("uv_grids/ash_uvgrid01.jpg").c_str() };

    if (!tldr) {
        return;
    }

    gl::CreateTextures(gl::TEXTURE_2D, 1, raw_handle_ptr(_objtex));
    gl::TextureStorage2D(raw_handle(_objtex), 1, tldr.internal_format(), tldr.width(), tldr.height());
    gl::TextureSubImage2D(
        raw_handle(_objtex), 0, 0, 0, tldr.width(), tldr.height(), tldr.format(), gl::UNSIGNED_BYTE, tldr.data());

    {
        gl::CreateTextures(gl::TEXTURE_2D, 1, raw_handle_ptr(_greentexture));
        gl::TextureStorage2D(raw_handle(_greentexture), 1, gl::RGBA8, 1, 1);

        const uint8_t clr[] = { 0, 255, 0, 0 };
        gl::ClearTexImage(raw_handle(_greentexture), 0, gl::RGBA, gl::UNSIGNED_BYTE, clr);
    }

    gl::CreateSamplers(1, raw_handle_ptr(_sampler));
    const auto smp = raw_handle(_sampler);
    gl::SamplerParameteri(smp, gl::TEXTURE_MIN_FILTER, gl::LINEAR);
    gl::SamplerParameteri(smp, gl::TEXTURE_MAG_FILTER, gl::LINEAR);

    gl::Enable(gl::DEPTH_TEST);
    gl::Enable(gl::CULL_FACE);

    _valid = true;
}

void
app::mesh_demo::draw(const float surface_width, const float surface_height)
{
    assert(valid());

    gl::ClearNamedFramebufferfv(0, gl::COLOR, 0, color_palette::web::black.components);
    gl::ClearNamedFramebufferfi(0, gl::DEPTH_STENCIL, 0, 1.0f, 0);
    gl::ViewportIndexedf(0, 0.0f, 0.0f, surface_width, surface_height);

    if (!_mesh_loaded) {
        return;
    }

    gl::BindVertexArray(raw_handle(_vao));

    struct transform_matrices
    {
        mat4f world_view_proj;
        mat4f normals;
        mat4f view;
    } tfmat;

    tfmat.view = _camera.view();
    tfmat.world_view_proj = _camera.projection_view();
    tfmat.normals = tfmat.view;

    _vs.set_uniform_block("TransformMatrices", tfmat);
    _fs.set_uniform("IMAGE_TEX", 0);

    _pipeline.use_vertex_program(_vs).use_fragment_program(_fs).disable_stage(render_stage::e::geometry).use();

    const GLuint bound_textures[] = { raw_handle(_objtex) };
    gl::BindTextures(0, 1, bound_textures);

    const GLuint bound_samplers[] = { raw_handle(_sampler) };
    gl::BindSamplers(0, 1, bound_samplers);

    {
        ScopedPolygonMode set_wireframe{ _drawparams.draw_wireframe ? gl::LINE : gl::FILL };

        gl::DrawElements(gl::TRIANGLES, _indexcount, gl::UNSIGNED_INT, nullptr);
    }

    if (_drawparams.drawnormals) {
        struct
        {
            mat4f WORLD_VIEW_PROJECTION;
            rgb_color COLOR_START;
            rgb_color COLOR_END;
            float NORMAL_LENGTH;
        } gs_uniforms;

        gs_uniforms.WORLD_VIEW_PROJECTION = tfmat.world_view_proj;
        gs_uniforms.COLOR_START = _drawparams.start_color;
        gs_uniforms.COLOR_END = _drawparams.end_color;
        gs_uniforms.NORMAL_LENGTH = _drawparams.normal_len;

        _gsnormals.set_uniform_block("TransformMatrices", gs_uniforms);

        _pipeline.use_vertex_program(_vsnormals)
            .use_geometry_program(_gsnormals)
            .use_fragment_program(_fsnormals)
            .use();

        gl::DrawElements(gl::TRIANGLES, _indexcount, gl::UNSIGNED_INT, nullptr);
    }

    if (_drawparams.draw_boundingbox) {
        draw_context_t dc;
        dc.window_width = static_cast<uint32_t>(surface_width);
        dc.window_height = static_cast<uint32_t>(surface_height);
        dc.view_matrix = _camera.view();
        dc.proj_view_matrix = _camera.projection_view();
        dc.active_camera = &_camera;
        _abbdraw.draw(dc, _bbox, color_palette::web::sea_green, 4.0f);
    }
}

void
app::mesh_demo::draw_ui(const int32_t surface_width, const int32_t surface_height)
{
    _ui->new_frame(surface_width, surface_height);

    ImGui::SetNextWindowPos({ 0.0f, 0.0f }, ImGuiCond_Appearing);

    if (ImGui::Begin("Options", nullptr, ImGuiWindowFlags_ShowBorders | ImGuiWindowFlags_AlwaysAutoResize)) {
        int32_t selected{ _sel_id == -1 ? 0 : _sel_id };
        ImGui::Combo(
            "Displayed model",
            &selected,
            [](void*, int32_t id, const char** out) {
                *out = loadable_meshes[id].display_name;
                return true;
            },
            nullptr,
            XR_I32_COUNTOF(loadable_meshes),
            5);

        if (selected != _sel_id) {
            XR_DBG_MSG("Changing mesh from {} to {}", _sel_id, selected);
            _sel_id = selected;

            _mesh_info = load_mesh(xr_app_config->model_path(loadable_meshes[_sel_id].file_name).c_str());
        }

        if (_mesh_info) {
            if (ImGui::CollapsingHeader("Mesh info", ImGuiTreeNodeFlags_Framed)) {
                const auto mi = _mesh_info.value();

                ImGui::Text("Vertex count: %u", mi.vertices);
                ImGui::Text("Index count: %u", mi.indices);
                ImGui::Text("Vertex bytes: %zu", mi.vertex_bytes);
                ImGui::Text("Index bytes: %zu", mi.index_bytes);
            }

            if (ImGui::CollapsingHeader("Debug options", ImGuiTreeNodeFlags_Framed)) {
                ImGui::Checkbox("Draw as wireframe", &_drawparams.draw_wireframe);
                ImGui::Checkbox("Draw bounding box", &_drawparams.draw_boundingbox);
                ImGui::Checkbox("Draw surface normals", &_drawparams.drawnormals);

                if (_drawparams.drawnormals) {
                    ImGui::ColorEdit3(
                        "Normal start color", _drawparams.start_color.components, ImGuiColorEditFlags_NoAlpha);
                    ImGui::ColorEdit3(
                        "Normal end color", _drawparams.end_color.components, ImGuiColorEditFlags_NoAlpha);
                    ImGui::SliderFloat("Normal length", &_drawparams.normal_len, 0.1f, 10.0f);
                }
            }
        }
    }

    ImGui::End();
    _ui->draw();
}

void
app::mesh_demo::event_handler(const xray::ui::window_event& evt)
{
    if (evt.type == event_type::configure) {
        const auto cfg = &evt.event.configure;
        _camera.set_projection(perspective_symmetric(
            static_cast<float>(cfg->width) / static_cast<float>(cfg->height), radians(70.0f), 0.1f, 100.0f));

        return;
    }

    if (is_input_event(evt)) {
        if (evt.type == event_type::key && evt.event.key.keycode == key_sym::e::escape) {
            _quit_receiver();
            return;
        }

        _ui->input_event(evt);
        if (!_ui->wants_input()) {
            _camcontrol.input_event(evt);
        }
    }
}

// void app::mesh_demo::poll_start(const xray::ui::poll_start_event&) {
//  _ui.input.begin();
//}

// void app::mesh_demo::poll_end(const xray::ui::poll_end_event&) {
//  _ui.input.end();
//}

void
app::mesh_demo::loop_event(const xray::ui::window_loop_event& wle)
{
    _camcontrol.update_camera(_camera);
    _ui->tick(1.0f / 60.0f);

    draw(static_cast<float>(wle.wnd_width), static_cast<float>(wle.wnd_height));
    draw_ui(wle.wnd_width, wle.wnd_height);
}

xray::base::maybe<app::mesh_demo::mesh_info>
app::mesh_demo::load_mesh(const char* model_path)
{
    auto loaded_obj = mesh_loader::load(model_path);
    if (!loaded_obj) {
        return nothing{};
    }

    const auto& mesh_loader = loaded_obj.value();

    if (mesh_loader.header().vertex_count > _vertexcount) {
        XR_DBG_MSG("Reallocating vertex buffer!");
        gl::NamedBufferData(raw_handle(_vb), mesh_loader.vertex_bytes(), mesh_loader.vertex_data(), gl::STREAM_DRAW);
    } else {
        scoped_resource_mapping srm{ raw_handle(_vb),
                                     gl::MAP_WRITE_BIT | gl::MAP_INVALIDATE_BUFFER_BIT,
                                     mesh_loader.vertex_bytes() };

        if (!srm) {
            XR_DBG_MSG("Failed to map vertex buffer for mapping!");
            return nothing{};
        }

        memcpy(srm.memory(), mesh_loader.vertex_data(), mesh_loader.vertex_bytes());
    }

    _vertexcount = mesh_loader.header().vertex_count;

    if (mesh_loader.header().index_count > _indexcount) {
        XR_DBG_MSG("Reallocating index buffer!");
        gl::NamedBufferData(raw_handle(_ib), mesh_loader.index_bytes(), mesh_loader.index_data(), gl::STREAM_DRAW);
    } else {
        scoped_resource_mapping srm{ raw_handle(_ib),
                                     gl::MAP_WRITE_BIT | gl::MAP_INVALIDATE_BUFFER_BIT,
                                     mesh_loader.index_bytes() };

        if (!srm) {
            XR_DBG_MSG("Failed to map index buffer for mapping");
            return nothing{};
        }

        memcpy(srm.memory(), mesh_loader.index_data(), mesh_loader.index_bytes());
    }

    _indexcount = mesh_loader.header().index_count;
    _bbox = mesh_loader.bounding().axis_aligned_bbox;
    _mesh_loaded = true;

    return mesh_info{ mesh_loader.header().vertex_count,
                      mesh_loader.header().index_count,
                      mesh_loader.vertex_bytes(),
                      mesh_loader.index_bytes(),
                      mesh_loader.bounding().axis_aligned_bbox,
                      mesh_loader.bounding().bounding_sphere };
}
