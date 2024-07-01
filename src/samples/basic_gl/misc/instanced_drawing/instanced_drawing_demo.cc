#include "misc/instanced_drawing/instanced_drawing_demo.hpp"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstring>
#include <filesystem>
#include <iterator>
#include <numeric>
#include <span>
#include <vector>

#include <imgui/imgui.h>
#include <itlib/small_vector.hpp>
#include <opengl/opengl.hpp>
#include <Lz/Lz.hpp>

#include "xray/base/app_config.hpp"
#include "xray/base/array_dimension.hpp"
#include "xray/math/constants.hpp"
#include "xray/math/objects/sphere.hpp"
#include "xray/math/objects/sphere_math.hpp"
#include "xray/math/projection.hpp"
#include "xray/math/scalar2.hpp"
#include "xray/math/scalar3_string_cast.hpp"
#include "xray/math/scalar4.hpp"
#include "xray/math/scalar4x4.hpp"
#include "xray/math/scalar4x4_math.hpp"
#include "xray/math/scalar4x4_string_cast.hpp"
#include "xray/math/transforms_r3.hpp"
#include "xray/math/transforms_r4.hpp"
#include "xray/rendering/colors/color_palettes.hpp"
#include "xray/rendering/draw_context.hpp"
#include "xray/rendering/geometry/geometry_data.hpp"
#include "xray/rendering/geometry/geometry_factory.hpp"
#include "xray/rendering/opengl/gl_misc.hpp"
#include "xray/rendering/opengl/indirect_draw_command.hpp"
#include "xray/rendering/opengl/scoped_opengl_setting.hpp"
#include "xray/rendering/opengl/scoped_resource_mapping.hpp"
#include "xray/rendering/opengl/scoped_state.hpp"
#include "xray/rendering/texture_loader.hpp"
#include "xray/ui/events.hpp"
#include "xray/ui/user_interface.hpp"

using namespace xray::base;
using namespace xray::rendering;
using namespace xray::math;
using namespace xray::ui;
using namespace std;

extern xray::base::ConfigSystem* xr_app_config;

struct rgba
{
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
};

tl::optional<app::SimpleWorld>
app::SimpleWorld::create(const init_context_t& init_ctx)
{
    texture_loader tl{ xr_app_config->texture_path("worlds/world1/seed_7677_elevation.png").c_str() };

    if (!tl) {
        XR_LOG_CRITICAL("Failed to open world file!");
        return tl::nullopt;
    }

    XR_LOG_INFO("World image {} {} {}", tl.width(), tl.height(), tl.depth());

    scoped_texture heightmap{ [&tl]() {
        GLuint tex_map{};
        gl::CreateTextures(gl::TEXTURE_2D, 1, &tex_map);
        gl::TextureStorage2D(tex_map, 1, tl.internal_format(), tl.width(), tl.height());
        gl::TextureSubImage2D(tex_map, 0, 0, 0, tl.width(), tl.height(), tl.format(), tl.pixel_size(), tl.data());
        return tex_map;
    }() };

    scoped_sampler sampler{ []() {
        GLuint smp{};
        gl::CreateSamplers(1, &smp);
        gl::SamplerParameteri(smp, gl::TEXTURE_MIN_FILTER, gl::LINEAR);
        gl::SamplerParameteri(smp, gl::TEXTURE_MAG_FILTER, gl::LINEAR);
        return smp;
    }() };

    const xray::math::vec2ui32 worldsize{ 1024, 1024 };
    const geometry_data_t geometry{ geometry_factory::grid(worldsize.x, worldsize.y, 1.0f, 1.0f) };

    XR_LOG_INFO("World grid: vertices {}, indices {}", geometry.vertex_count, geometry.index_count);

    vector<vertex_pnt> vertices{};
    vertices.reserve(geometry.vertex_count);
    transform(raw_ptr(geometry.geometry),
              raw_ptr(geometry.geometry) + geometry.vertex_count,
              back_inserter(vertices),
              [](const vertex_pntt& vtx) {
                  return vertex_pnt{ vtx.position, vtx.normal, vtx.texcoords };
              });

    basic_mesh world{ std::span{ vertices },
                      std::span{ raw_ptr(geometry.indices), raw_ptr(geometry.indices) + geometry.index_count } };

    vertex_program vs{ gpu_program_builder{}
                           .push_file_block(init_ctx.cfg->shader_path("misc/instanced_draw/vs.world.glsl"))
                           .build_ex<render_stage::e::vertex>() };
    if (!vs)
        return tl::nullopt;

    fragment_program fs{ gpu_program_builder{}
                             .push_file_block(init_ctx.cfg->shader_path("misc/instanced_draw/fs.world.glsl"))
                             .build_ex<render_stage::e::fragment>() };
    if (!fs)
        return tl::nullopt;

    program_pipeline pipeline{ []() {
        GLuint pp{};
        gl::CreateProgramPipelines(1, &pp);
        return pp;
    }() };

    pipeline.use_vertex_program(vs).use_fragment_program(fs);

    return tl::make_optional<SimpleWorld>(std::move(world),
                                          std::move(vs),
                                          std::move(fs),
                                          std::move(pipeline),
                                          std::move(heightmap),
                                          std::move(sampler),
                                          worldsize);
}

void
app::SimpleWorld::update(const float)
{
}

void
app::SimpleWorld::draw(const xray::scene::camera* cam)
{
    _vs.set_uniform("WORLD_VIEW_PROJ", cam->projection_view());
    _vs.flush_uniforms();

    gl::BindVertexArray(_world.vertex_array());
    _pp.use(false);
    gl::BindTextureUnit(0, raw_handle(_heightmap));
    gl::BindSampler(0, raw_handle(_sampler));
    gl::DrawElements(gl::TRIANGLES, _world.index_count(), gl::UNSIGNED_INT, nullptr);
}

app::InstancedDrawingDemo::InstancedDrawingDemo(PrivateConstructionToken,
                                                const init_context_t& init_ctx,
                                                RenderState rs,
                                                InstancingState ins)
    : DemoBase(init_ctx)
    , _obj_instances(std::move(ins))
    , _render_state(std::move(rs))
{
    //
    // turn off these so we don't get spammed
    gl::DebugMessageControl(gl::DONT_CARE, gl::DONT_CARE, gl::DEBUG_SEVERITY_NOTIFICATION, 0, nullptr, gl::FALSE_);

    xray::scene::camera_lens_parameters lens_param;
    lens_param.aspect_ratio = static_cast<float>(init_ctx.surface_width) / static_cast<float>(init_ctx.surface_height);
    lens_param.nearplane = 0.1f;
    lens_param.farplane = 1000.0f;
    lens_param.fov = radians(70.0f);
    _scene.cam_control.set_lens_parameters(lens_param);

    gl::Enable(gl::DEPTH_TEST);
    gl::Enable(gl::CULL_FACE);
    gl::CullFace(gl::BACK);
    gl::FrontFace(gl::CCW);
    gl::ViewportIndexedf(
        0, 0.0f, 0.0f, static_cast<float>(init_ctx.surface_width), static_cast<float>(init_ctx.surface_height));

    _render_state._timer.start();
}

xray::base::unique_pointer<app::DemoBase>
app::InstancedDrawingDemo::create(const init_context_t& initContext)
{
    tl::optional<SimpleWorld> sw{ SimpleWorld::create(initContext) };
    if (!sw)
        return nullptr;

    itlib::small_vector<mesh_loader, 4> mloaders;

    lz::chain(std::initializer_list<const char*>{ "f15/f15c.bin", "typhoon/typhoon.bin" })
        .map([init_ctx = &initContext](const char* mdl_file) {
            return mesh_loader::load(init_ctx->cfg->model_path(mdl_file).c_str());
        })
        .filter([](auto&& ml) { return ml != tl::nullopt; })
        .map([](auto&& ml) { return *ml.take(); })
        .copyTo(std::back_inserter(mloaders));

    if (mloaders.empty()) {
        XR_LOG_CRITICAL("Could not load a single model file ...");
    }

    const auto [vertex_bytes, index_bytes] = accumulate(
        cbegin(mloaders),
        cend(mloaders),
        pair<size_t, size_t>{ 0, 0 },
        [](const pair<size_t, size_t> acc, const mesh_loader& loaded_mdl) {
            return pair<size_t, size_t>{ acc.first + loaded_mdl.vertex_bytes(), acc.second + loaded_mdl.index_bytes() };
        });

    scoped_buffer vertex_buffer{ [=, &mloaders]() {
        GLuint vertex_buffer{};
        gl::CreateBuffers(1, &vertex_buffer);
        gl::NamedBufferStorage(vertex_buffer, vertex_bytes, 0, gl::MAP_WRITE_BIT);

        // scoped_resource_mapping::map(scoped_buffer &sb, const uint32_t access, const size_t length)
        scoped_resource_mapping bmap{ vertex_buffer, gl::MAP_WRITE_BIT, vertex_bytes };

        if (!bmap) {
            XR_LOG_CRITICAL("Failed to map vertex buffer for writing!");
            return vertex_buffer;
        }

        {
            const auto ignored =
                accumulate(begin(mloaders),
                           end(mloaders),
                           size_t{ 0u },
                           [dst = bmap.memory()](const size_t offset, const mesh_loader& ml) {
                               memcpy(static_cast<uint8_t*>(dst) + offset, ml.vertex_data(), ml.vertex_bytes());
                               return offset + ml.vertex_bytes();
                           });
        }

        GL_MARK_BUFFER(vertex_buffer, "Combined vertex buffer");

        return vertex_buffer;
    }() };

    scoped_buffer index_buffer{ [=, &mloaders]() {
        GLuint index_buffer{};
        gl::CreateBuffers(1, &index_buffer);
        gl::NamedBufferStorage(index_buffer, index_bytes, nullptr, gl::MAP_WRITE_BIT);

        scoped_resource_mapping imap{ index_buffer, gl::MAP_WRITE_BIT, index_bytes };

        if (!imap) {
            XR_LOG_ERR("Failed to map index buffer for writing!");
            return index_buffer;
        }

        {
            const auto ignored =
                accumulate(begin(mloaders),
                           end(mloaders),
                           size_t{ 0u },
                           [dst = imap.memory()](const size_t offset, const mesh_loader& ml) {
                               memcpy(static_cast<uint8_t*>(dst) + offset, ml.index_data(), ml.index_bytes());
                               return offset + ml.index_bytes();
                           });
        }

        GL_MARK_BUFFER(index_buffer, "Combined index buffer");

        return index_buffer;
    }() };

    const auto draw_cmds_data =
        lz::chain(mloaders)
            .map([acc = draw_elements_indirect_command{ 0, 0, 0, 0, 0 }](const mesh_loader& mdl) mutable {
                const draw_elements_indirect_command draw_cmd{
                    .count = mdl.header().index_count,
                    .instance_count = 16,
                    .first_index = acc.count,
                    .base_vertex = acc.base_vertex,
                    .base_instance = acc.instance_count,
                };

                acc.count += mdl.header().index_count;
                acc.instance_count += 16;
                acc.base_vertex += mdl.header().vertex_count;

                return draw_cmd;
            })
            .to<itlib::small_vector<draw_elements_indirect_command, 4>>();

    scoped_buffer draw_cmds_buffer{ [&draw_cmds_data]() {
        GLuint cmd_buff{};

        gl::CreateBuffers(1, &cmd_buff);
        gl::NamedBufferStorage(cmd_buff, container_bytes_size(draw_cmds_data), draw_cmds_data.data(), 0);

        GL_MARK_BUFFER(cmd_buff, "Indirect draw commands buffer");
        return cmd_buff;
    }() };

    scoped_vertex_array vertexarray{
        [vb = raw_handle(vertex_buffer), ib = raw_handle(index_buffer), di = raw_handle(draw_cmds_buffer)]() {
            GLuint vao{};
            gl::CreateVertexArrays(1, &vao);

            gl::VertexArrayVertexBuffer(vao, 0, vb, 0, sizeof(vertex_pnt));
            gl::VertexArrayElementBuffer(vao, ib);

            gl::VertexArrayAttribFormat(vao, 0, 3, gl::FLOAT, gl::FALSE_, offsetof(vertex_pnt, position));
            gl::VertexArrayAttribFormat(vao, 1, 3, gl::FLOAT, gl::FALSE_, offsetof(vertex_pnt, normal));
            gl::VertexArrayAttribFormat(vao, 2, 2, gl::FLOAT, gl::FALSE_, offsetof(vertex_pnt, texcoord));

            gl::VertexArrayAttribBinding(vao, 0, 0);
            gl::VertexArrayAttribBinding(vao, 1, 0);
            gl::VertexArrayAttribBinding(vao, 2, 0);

            gl::EnableVertexArrayAttrib(vao, 0);
            gl::EnableVertexArrayAttrib(vao, 1);
            gl::EnableVertexArrayAttrib(vao, 2);

            return scoped_vertex_array{ vao };
        }()
    };

    vertex_program vert_shader{ gpu_program_builder{}
                                    .push_file_block(initContext.cfg->shader_path("misc/instanced_draw/vs.glsl"))
                                    .build_ex<render_stage::e::vertex>() };

    if (!vert_shader) {
        return nullptr;
    }

    fragment_program frag_shader{ gpu_program_builder{}
                                      .push_file_block(initContext.cfg->shader_path("misc/instanced_draw/fs.glsl"))
                                      .build_ex<render_stage::e::fragment>() };

    if (!frag_shader) {
        return nullptr;
    }

    scoped_program_pipeline_handle pipeline{ create_program_pipeline(vert_shader, frag_shader) };

    //
    // load textures
    scoped_texture textures{ [&initContext]() {
        GLuint tex{};
        gl::CreateTextures(gl::TEXTURE_2D_ARRAY, 1, &tex);

        namespace fs = std::filesystem;

        itlib::small_vector<fs::path, 4> texture_files;
        for (const fs::directory_entry& de :
             fs::recursive_directory_iterator{ initContext.cfg->texture_path("uv_grids") }) {
            if (!de.is_regular_file())
                continue;

            texture_files.emplace_back(de.path());
        }

        if (texture_files.empty()) {
            XR_LOG_CRITICAL("No textures found ...");
            return 0u;
        }

        std::ranges::for_each(
            texture_files,
            [tex, tex_storage_alloc = false, tex_idx = 0u, tex_count = texture_files.size()](
                const fs::path& tex_file) mutable {
                texture_loader tl{ tex_file.c_str(), texture_load_options::flip_y };
                if (!tl) {
                    return;
                }

                if (!tex_storage_alloc) {
                    gl::TextureStorage3D(tex, 1, tl.internal_format(), tl.width(), tl.height(), tex_count);
                    tex_storage_alloc = true;
                }

                gl::TextureSubImage3D(
                    tex, 0, 0, 0, tex_idx++, tl.width(), tl.height(), 1, tl.format(), gl::UNSIGNED_BYTE, tl.data());
            });

        return tex;
    }() };

    //
    // Create instances
    random_number_generator rng{};
    const vector<instance_info> instances{ lz::chain(lz::range(InstancingState::instance_count))
                                               .map([r = &rng](const uint32_t idx) {
                                                   static constexpr auto OBJ_DST = 50.0f;

                                                   instance_info new_inst;
                                                   new_inst.pitch = r->next_float(0.0f, two_pi<float>);
                                                   new_inst.roll = r->next_float(0.0f, two_pi<float>);
                                                   new_inst.yaw = r->next_float(0.0f, two_pi<float>);
                                                   new_inst.scale = idx < 16 ? 1.0f : 1.0f;
                                                   new_inst.texture_id = r->next_uint(0, 10);
                                                   new_inst.position = vec3f{ r->next_float(-OBJ_DST, +OBJ_DST),
                                                                              r->next_float(OBJ_DST, +2.0f * OBJ_DST),
                                                                              r->next_float(-OBJ_DST, +OBJ_DST) };

                                                   return new_inst;
                                               })
                                               .to<vector<instance_info>>() };

    scoped_buffer buffer_transforms{ [byte_size = instances.size() * sizeof(InstanceXData)]() {
        XR_LOG_INFO("Creating instance transform buffer, byte size {}", byte_size);
        GLuint buff{};
        gl::CreateBuffers(1, &buff);
        gl::NamedBufferStorage(buff, byte_size, nullptr, gl::MAP_WRITE_BIT);

        return buff;
    }() };

    scoped_sampler sampler = []() {
        GLuint smp{};
        gl::CreateSamplers(1, &smp);
        gl::SamplerParameteri(smp, gl::TEXTURE_MIN_FILTER, gl::LINEAR);
        gl::SamplerParameteri(smp, gl::TEXTURE_MAG_FILTER, gl::LINEAR);

        return scoped_sampler{ smp };
    }();

    return xray::base::make_unique<InstancedDrawingDemo>(
        PrivateConstructionToken{},
        initContext,
        RenderState{ std::move(vertex_buffer),
                     std::move(index_buffer),
                     std::move(draw_cmds_buffer),
                     std::move(vertexarray),
                     std::move(vert_shader),
                     std::move(frag_shader),
                     std::move(pipeline),
                     std::move(textures),
                     std::move(sampler),
                     vec2i32{ initContext.surface_width, initContext.surface_height },
                     timer_highp{},
                     std::move(*sw.take()) },
        InstancingState{ std::move(instances), std::move(buffer_transforms) });
}

app::InstancedDrawingDemo::~InstancedDrawingDemo() {}

void
app::InstancedDrawingDemo::loop_event(const app::RenderEvent& render_event)
{
    const xray::ui::window_loop_event& wle = render_event.loop_event;
    //
    // process input
    _scene.cam_control.process_input(_keyboard);

    RenderState* r = &_render_state;
    Scene* s = &_scene;
    InstancingState* is = &_obj_instances;

    r->_timer.update_and_reset();
    const auto delta_tm = r->_timer.elapsed_millis();

    _ui->tick(delta_tm);

    s->cam_control.update_camera(s->camera);

    gl::ClearNamedFramebufferfv(0, gl::COLOR, 0, color_palette::web::black.components);
    gl::ClearNamedFramebufferfi(0, gl::DEPTH_STENCIL, 0, 1.0f, 0);

    r->_world.draw(&s->camera);

    {
        lz::chain(is->instances)
            .map([pv = s->camera.projection_view()](instance_info& ii) {
                ii.pitch += instance_info::rotation_speed;
                ii.roll += instance_info::rotation_speed;
                ii.yaw += instance_info::rotation_speed;

                if (ii.pitch > two_pi<float>) {
                    ii.pitch -= two_pi<float>;
                }

                if (ii.roll > two_pi<float>) {
                    ii.roll -= two_pi<float>;
                }

                if (ii.yaw > two_pi<float>) {
                    ii.yaw -= two_pi<float>;
                }
                const auto obj_rotation = mat4f{ R3::rotate_xyz(ii.roll, ii.yaw, ii.pitch) };

                return InstanceXData{ pv * R4::translate(ii.position) * obj_rotation * mat4f{ R3::scale(ii.scale) },
                                      obj_rotation };
            })
            .copyTo(is->scratch_buffer.begin());

        ScopedResourceMapping::create(raw_handle(is->buffer_transforms),
                                      gl::MAP_WRITE_BIT | gl::MAP_INVALIDATE_RANGE_BIT,
                                      container_bytes_size(is->scratch_buffer))
            .map_or_else(
                [is](ScopedResourceMapping bmap) {
                    memcpy(bmap.memory(), is->scratch_buffer.data(), container_bytes_size(is->scratch_buffer));
                },
                [is]() {
                    XR_LOG_ERR("Failed to map instance transform buffer, error: {:#x}, cont bytes {}",
                               gl::GetError(),
                               container_bytes_size(is->scratch_buffer));
                });
    }

    gl::ViewportIndexedf(0, 0.0f, 0.0f, static_cast<float>(wle.wnd_width), static_cast<float>(wle.wnd_height));

    const GLuint ssbos[] = { raw_handle(is->buffer_transforms) };

    gl::BindBuffersBase(gl::SHADER_STORAGE_BUFFER, 0, size(ssbos), ssbos);

    gl::BindVertexArray(raw_handle(r->_vertexarray));
    gl::BindTextureUnit(0, raw_handle(r->_textures));
    gl::BindSampler(0, raw_handle(r->_sampler));
    gl::BindProgramPipeline(raw_handle(r->_pipeline));

    scoped_indirect_draw_buffer_binding draw_indirect_buffer_binding{ raw_handle(r->_indirect_draw_cmd_buffer) };

    ScopedWindingOrder front_faces{ gl::CW };
    gl::MultiDrawElementsIndirect(gl::TRIANGLES, gl::UNSIGNED_INT, nullptr, 2, 0);

    compose_ui(wle.wnd_width, wle.wnd_height);
}

void
app::InstancedDrawingDemo::compose_ui(const int32_t surface_width, const int32_t surface_height)
{
    _ui->new_frame(surface_width, surface_height);
    ImGui::SetNextWindowPos({ 0.0f, 0.0f }, ImGuiCond_Appearing);

    if (ImGui::Begin("Options", nullptr, ImGuiWindowFlags_None)) {
        // if (ImGui::SliderInt("Instance count",
        //                      &_demo_opts.instance_count,
        //                      2,
        //                      static_cast<int32_t>(InstancingState::instance_count))) {
        //
        //     scoped_resource_mapping draw_cmd_buffer{ raw_handle(_indirect_draw_cmd_buffer),
        //                                              gl::MAP_WRITE_BIT,
        //                                              sizeof(draw_elements_indirect_command) * 2 };
        //
        //     if (draw_cmd_buffer) {
        //         auto ptr = static_cast<draw_elements_indirect_command*>(draw_cmd_buffer.memory());
        //
        //         ptr[0].instance_count = _demo_opts.instance_count / 2;
        //         ptr[1].instance_count = _demo_opts.instance_count / 2;
        //     }
        // }

        if (ImGui::CollapsingHeader("::: Object instances :::")) {

            array<char, 512> tmp_buf{};

            lz::chain(lz::enumerate(_obj_instances.instances)).forEach([&](auto&& idx_instance_pair) {
                auto&& [idx, ii] = idx_instance_pair;

                const uintptr_t node_id{ reinterpret_cast<uintptr_t>(&idx_instance_pair.second) };

                if (ImGui::TreeNodeEx(reinterpret_cast<const void*>(node_id),
                                      ImGuiTreeNodeFlags_CollapsingHeader,
                                      "Instance #%u",
                                      idx)) {

                    ImGui::SliderFloat("Roll", &ii.roll, -two_pi<float>, two_pi<float>);
                    ImGui::SliderFloat("Pitch", &ii.pitch, -two_pi<float>, two_pi<float>);
                    ImGui::SliderFloat("Yaw", &ii.yaw, -two_pi<float>, two_pi<float>);
                    ImGui::InputFloat("Scaling", &ii.scale, 0.1f, 0.5f, "%.3f", ImGuiInputTextFlags_CharsDecimal);

                    ImGui::Separator();
                    const auto [out_it, cch] =
                        fmt::format_to_n(tmp_buf.data(), tmp_buf.size(), "Position: {:>3.3f}", ii.position);
                    tmp_buf[cch] = 0;
                    ImGui::TextColored({ 0.0f, 1.0f, 0.0f, 1.0f }, "%s", tmp_buf.data());

                    ii.scale = xray::math::clamp(ii.scale, 0.1f, 10.0f);
                }
            });
        }
    }

    ImGui::End();
    _ui->draw();
}

void
app::InstancedDrawingDemo::event_handler(const xray::ui::window_event& evt)
{
    DemoBase::event_handler(evt);

    if (evt.type == event_type::configure) {
        RenderState* r = &_render_state;
        r->_window_size.x = evt.event.configure.width;
        r->_window_size.y = evt.event.configure.height;

        xray::scene::camera_lens_parameters lp;
        lp.aspect_ratio =
            static_cast<float>(evt.event.configure.width) / static_cast<float>(evt.event.configure.height);
        lp.farplane = 1000.0f;
        lp.nearplane = 0.1f;
        lp.fov = radians(70.0f);

        _scene.cam_control.set_lens_parameters(lp);

        gl::ViewportIndexedf(0,
                             0.0f,
                             0.0f,
                             static_cast<float>(evt.event.configure.width),
                             static_cast<float>(evt.event.configure.height));

        return;
    }

    if (is_input_event(evt)) {
        _ui->input_event(evt);
        if (!_ui->wants_input()) {
            //
            //  Quit on escape
            if (evt.type == event_type::key && evt.event.key.keycode == KeySymbol::escape) {
                _quit_receiver();
                return;
            }

            _scene.cam_control.input_event(evt);
        }
        return;
    }
}
