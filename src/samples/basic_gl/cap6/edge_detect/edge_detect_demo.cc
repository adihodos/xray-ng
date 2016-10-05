#include "cap6/edge_detect/edge_detect_demo.hpp"
#include "helpers.hpp"
#include "init_context.hpp"
#include "mtl_component_type.hpp"
#include "resize_context.hpp"
#include "xray/base/app_config.hpp"
#include "xray/base/config_settings.hpp"
#include "xray/base/containers/fixed_stack.hpp"
#include "xray/base/containers/fixed_vector.hpp"
#include "xray/base/dbg/debug_ext.hpp"
#include "xray/base/enum_cast.hpp"
#include "xray/base/fast_delegate.hpp"
#include "xray/base/fnv_hash.hpp"
#include "xray/base/logger.hpp"
#include "xray/base/pod_zero.hpp"
#include "xray/base/scoped_guard.hpp"
#include "xray/base/shims/attribute/basic_path.hpp"
#include "xray/base/shims/string.hpp"
#include "xray/math/constants.hpp"
#include "xray/math/math_std.hpp"
#include "xray/math/projection.hpp"
#include "xray/math/scalar2.hpp"
#include "xray/math/scalar3_math.hpp"
#include "xray/math/scalar3x3_math.hpp"
#include "xray/math/scalar4x4_math.hpp"
#include "xray/math/transforms_r3.hpp"
#include "xray/math/transforms_r4.hpp"
#include "xray/rendering/colors/color_palettes.hpp"
#include "xray/rendering/draw_context.hpp"
#include "xray/rendering/geometry/geometry_data.hpp"
#include "xray/rendering/geometry/geometry_factory.hpp"
#include "xray/rendering/opengl/scoped_state.hpp"
#include "xray/rendering/opengl/shader_base.hpp"
#include "xray/rendering/texture_loader.hpp"
#include "xray/rendering/vertex_format/vertex_pc.hpp"
#include "xray/rendering/vertex_format/vertex_pn.hpp"
#include "xray/scene/camera.hpp"
#include "xray/scene/config_reader_rgb_color.hpp"
#include "xray/scene/config_reader_scene.hpp"
#include "xray/scene/point_light.hpp"
#include <algorithm>
#include <array>
#include <imgui/imgui.h>
#include <platformstl/filesystem/memory_mapped_file.hpp>
#include <platformstl/filesystem/path.hpp>
#include <platformstl/filesystem/path_functions.hpp>
#include <stlsoft/memory/auto_buffer.hpp>
#include <tbb/tbb.h>
#include <unordered_map>
#include <vector>

extern xray::base::app_config* xr_app_config;

using namespace xray::base;
using namespace xray::math;
using namespace xray::rendering;
using namespace xray::scene;
using namespace std;
using namespace app;

struct mtl_file_desc {
  const char* path;
  bool        flip_y;
};

struct mtl_color_desc {
  xray::rendering::rgb_color clr;
};

enum class mtl_entry_type { tex, color };

struct mtl_desc_entry {
  mtl_component_type::e component_type;
  mtl_entry_type        source_type;

  union {
    mtl_file_desc  file;
    mtl_color_desc clr;
  };

  mtl_desc_entry() = default;

  mtl_desc_entry(const mtl_component_type::e comp_type, const mtl_file_desc& fd)
      : component_type{comp_type}, source_type{mtl_entry_type::tex}, file{fd} {}

  mtl_desc_entry(const mtl_component_type::e comp_type,
                 const mtl_color_desc&       clr_)
      : component_type{comp_type}
      , source_type{mtl_entry_type::color}
      , clr{clr_} {}
};

struct material {
  xray::rendering::scoped_texture diffuse;
  xray::rendering::scoped_texture specular;
  xray::rendering::scoped_texture emissive;
  xray::rendering::scoped_texture ambient;
  float                           spec_pwr{100.0f};

  material() = default;
  XRAY_DEFAULT_MOVE(material);

private:
  XRAY_NO_COPY(material);
};

class scene_loader {
public:
  explicit scene_loader(const char* scene_def_file);

  explicit operator bool() const noexcept { return valid(); }
  bool valid() const noexcept { return _scene_file.valid(); }

  xray::rendering::gpu_program_builder load_program_description(const char* id);

  void load_material_description(
      const char* id, xray::base::fast_delegate<void(const mtl_desc_entry&)>
                          mtl_entry_parsed_event);

  static material create_material(const mtl_desc_entry* descriptions,
                                  const size_t          count);

private:
  void read_program_entry(const char*                           id,
                          xray::rendering::gpu_program_builder* prg_bld);

private:
  using path_stack_type =
      xray::base::fixed_stack<platformstl::basic_path<char>, 4>;

  static constexpr auto PRG_SEC_NAME = "app.scene.programs";
  static constexpr auto MTL_SEC_NAME = "app.scene.materials";

  xray::base::config_file  _scene_file;
  xray::base::config_entry _prg_sec;
  path_stack_type          _prg_root_path;
  xray::base::config_entry _mtl_sec;
};

scene_loader::scene_loader(const char* scene_desc_file)
    : _scene_file{scene_desc_file}
    , _prg_sec{_scene_file.lookup_entry(PRG_SEC_NAME)}
    , _mtl_sec{_scene_file.lookup_entry(MTL_SEC_NAME)} {
  assert(_prg_sec);

  if (const auto top_root = _prg_sec.lookup_string("root_path")) {
    _prg_root_path.emplace(top_root.value());
  }
}

xray::rendering::gpu_program_builder
scene_loader::load_program_description(const char* id) {
  assert(id != nullptr);

  using namespace xray::rendering;

  gpu_program_builder prg_builder{};
  read_program_entry(id, &prg_builder);

  return prg_builder;
}

void scene_loader::read_program_entry(
    const char* id, xray::rendering::gpu_program_builder* prg_bld) {

  assert(valid());
  assert(id != nullptr);
  assert(prg_bld != nullptr);

  const auto prg_entry = _prg_sec.lookup(id);
  if (!prg_entry) {
    XR_LOG_CRITICAL("No program [[{}]] in program section", id);
    XR_NOT_REACHED();
  }

  const bool pop_path = [this]() {
    if (const auto custom_root = _prg_sec.lookup_string("root_path")) {
      _prg_root_path.emplace(custom_root.value());
      return true;
    }

    return false;
  }();

  XRAY_SCOPE_EXIT {
    if (pop_path)
      _prg_root_path.pop();
  };

  using namespace std;
  using namespace platformstl;
  using namespace xray::base;
  using namespace xray::rendering;

  using std::int32_t;
  using std::uint32_t;

  if (const auto strs = prg_entry.lookup("strings")) {
    assert(strs.is_array());

    for (uint32_t si = 0, si_cnt = strs.length(); si < si_cnt; ++si) {
      prg_bld->add_string(strs[si].as_string());
    }
  }

  //
  // files section
  if (const auto files_sec = prg_entry.lookup("files")) {
    assert(files_sec.is_array());

    //
    // this is fcking horrible, but builder only stores a pointer to the
    // string :(
    static vector<basic_path<char>> scratch_buffer;
    scratch_buffer.clear();
    //
    // Make sure adding items does not cause a reallocation of the vector's
    // data store, otherwise program_builder will get pointers to invalid data.
    scratch_buffer.reserve(files_sec.length());

    for (uint32_t idx = 0, files_cnt = files_sec.length(); idx < files_cnt;
         ++idx) {

      basic_path<char> full_path;
      if (!_prg_root_path.empty()) {
        full_path.push(_prg_root_path.top());
      }

      const auto fpath = basic_path<char>{files_sec[idx].as_string()};
      full_path.push(fpath);
      scratch_buffer.push_back(move(full_path));

      prg_bld->add_file(scratch_buffer.back().c_str());
    }
  }
}

void scene_loader::load_material_description(
    const char* id, xray::base::fast_delegate<void(const mtl_desc_entry&)>
                        mtl_entry_parsed_event) {
  assert(_mtl_sec && "No material section defined in config file!");
  assert(mtl_entry_parsed_event && "Invalid parse event delegate!");
  assert(id != nullptr);

  const auto msec = _mtl_sec.lookup(id);
  if (!msec) {
    XR_LOG_CRITICAL("Material {} not defined!", id);
    XR_NOT_REACHED();
  }

  using namespace std;

  for_each(mtl_component_type::cbegin(), mtl_component_type::cend(),
           [&msec, id,
            mtl_entry_parsed_event](const mtl_component_type::e mtl_type) {

             const auto def_sec =
                 msec.lookup(mtl_component_type::name(mtl_type));

             if (!def_sec) {
               if (mtl_type == mtl_component_type::e::diffuse) {
                 XR_LOG_CRITICAL("Missing diffuse entry is mandatory for "
                                 "material {} definition!",
                                 id);
                 XR_NOT_REACHED();
               }

               return;
             }

             if (def_sec.is_array()) {
               const auto color = [&def_sec]() {
                 assert(def_sec.length() == 3);
                 return rgb_color{def_sec.float_at(0) / 255.0f,
                                  def_sec.float_at(1) / 255.0f,
                                  def_sec.float_at(2) / 255.0f};
               }();

               mtl_entry_parsed_event(
                   mtl_desc_entry{mtl_type, mtl_color_desc{color}});
               return;
             }

             const auto fn = def_sec.lookup("file");
             const auto fy = def_sec.lookup("flip_y");

             mtl_entry_parsed_event(mtl_desc_entry{
                 mtl_type, mtl_file_desc{fn.as_string(), fy.as_bool()}});
           });
}

material scene_loader::create_material(const mtl_desc_entry* descriptions,
                                       const size_t          count) {
  material mtl;
  auto tex_assign_fn = [&mtl](scoped_texture              tex_handle,
                              const mtl_component_type::e comp_type) {
    switch (comp_type) {
    case mtl_component_type::e::ambient:
      mtl.ambient = std::move(tex_handle);
      break;

    case mtl_component_type::e::diffuse:
      mtl.diffuse = std::move(tex_handle);
      break;

    case mtl_component_type::e::emissive:
      mtl.diffuse = std::move(tex_handle);
      break;

    case mtl_component_type::e::specular:
      mtl.specular = std::move(tex_handle);
      break;

    default:
      XR_LOG_CRITICAL("Unsupported component for material creation!");
      XR_NOT_REACHED();
      break;
    }
  };

  for_each(descriptions, descriptions + count, [tex_assign_fn](
                                                   const mtl_desc_entry&
                                                       mtl_desc) {

    scoped_texture texh{};
    gl::CreateTextures(gl::TEXTURE_2D, 1, raw_handle_ptr(texh));

    if (mtl_desc.source_type == mtl_entry_type::color) {
      gl::TextureStorage2D(raw_handle(texh), 1, gl::RGBA8, 1, 1);
      gl::TextureSubImage2D(raw_handle(texh), 0, 0, 0, 1, 1, gl::RGBA,
                            gl::FLOAT, mtl_desc.clr.clr.components);
      tex_assign_fn(std::move(texh), mtl_desc.component_type);
      return;
    }

    texture_loader tex_ldr{
        c_str_ptr(xr_app_config->texture_path(mtl_desc.file.path)),
        mtl_desc.file.flip_y ? texture_load_options::flip_y
                             : texture_load_options::none};

    if (!tex_ldr) {
      XR_NOT_REACHED();
    }

    static constexpr GLenum TEXTURE_INTERNAL_FORMAT[] = {gl::RGB8, gl::RGBA8};

    gl::TextureStorage2D(raw_handle(texh), 1,
                         TEXTURE_INTERNAL_FORMAT[tex_ldr.depth() == 4],
                         static_cast<GLsizei>(tex_ldr.width()),
                         static_cast<GLsizei>(tex_ldr.height()));

    static constexpr GLenum IMAGE_PIXEL_FORMAT[] = {gl::RGB, gl::RGBA};

    gl::TextureSubImage2D(raw_handle(texh), 0, 0, 0,
                          static_cast<GLsizei>(tex_ldr.width()),
                          static_cast<GLsizei>(tex_ldr.height()),
                          IMAGE_PIXEL_FORMAT[tex_ldr.depth() == 4],
                          gl::UNSIGNED_BYTE, tex_ldr.data());

    tex_assign_fn(std::move(texh), mtl_desc.component_type);
  });

  return mtl;
}

///
///
/// scene loader ends

app::edge_detect_demo::edge_detect_demo(const init_context_t& init_ctx) {
  init(init_ctx);
}

app::edge_detect_demo::~edge_detect_demo() {}

void app::edge_detect_demo::compose_ui() {
  if (ImGui::Begin("Settings", nullptr, ImGuiWindowFlags_AlwaysAutoResize |
                                            ImGuiWindowFlags_NoMove)) {

    if (ImGui::RadioButton("No edge detection",
                           (_detect_method == edge_detect_none))) {
      _detect_method = edge_detect_none;
    }

    if (ImGui::RadioButton("Sobel edge detection",
                           (_detect_method == edge_detect_sobel))) {
      _detect_method = edge_detect_sobel;
    }

    if (ImGui::RadioButton("Frei-Chen edge detection",
                           (_detect_method == edge_detect_frei_chen))) {
      _detect_method = edge_detect_frei_chen;
    }

    if (_detect_method == edge_detect_sobel) {
      ImGui::Separator();
      ImGui::SliderFloat("Edge treshold", &_edge_treshold, 0.01f, 0.25f,
                         "%3.3f", 0.1f);
    }

    ImGui::Checkbox("Rotate model", &_rotate_object);
  }
  ImGui::End();
}

void app::edge_detect_demo::draw(const xray::rendering::draw_context_t& dc) {
  assert(valid());

  {
    //
    // first pass - normal phong lighting
    gl::BindFramebuffer(gl::FRAMEBUFFER, raw_handle(_fbo.fbo_object));

    {
      const float viewport_dim[] = {0.0f, 0.0f,
                                    static_cast<float>(dc.window_width),
                                    static_cast<float>(dc.window_height)};

      gl::ViewportIndexedfv(0, viewport_dim);
      gl::ClearNamedFramebufferfv(raw_handle(_fbo.fbo_object), gl::COLOR, 0,
                                  color_palette::web::black.components);
      const float def_clear_depthval{1.0f};
      gl::ClearNamedFramebufferfv(raw_handle(_fbo.fbo_object), gl::DEPTH, 0,
                                  &def_clear_depthval);
    }

    {
      const GLuint bound_textures[] = {raw_handle(_fbo.fbo_texture),
                                       raw_handle(_obj_material),
                                       raw_handle(_obj_material)};

      gl::BindTextures(0, XR_U32_COUNTOF__(bound_textures), bound_textures);
    }

    {
      const GLuint bound_samplers[] = {raw_handle(_fbo.fbo_sampler),
                                       raw_handle(_sampler_obj),
                                       raw_handle(_sampler_obj)};

      gl::BindSamplers(0, XR_U32_COUNTOF__(bound_samplers), bound_samplers);
    }

    const auto obj_to_world = float4x4{R3::rotate_xyz(_rx, _ry, _rz)};
    // float4x4::stdc::identity;
    const auto obj_to_view = dc.view_matrix * obj_to_world;

    struct matrix_pack_t {
      float4x4 world_view;
      float4x4 normal_view;
      float4x4 world_view_proj;
    } const obj_transforms{obj_to_view, obj_to_view,
                           dc.projection_matrix * obj_to_view};

    _vertex_prg.set_uniform_block("transform_pack", obj_transforms);

    point_light lights[edge_detect_demo::max_lights];
    transform(begin(_lights), end(_lights), begin(lights),
              [&dc](const auto& in_light) -> point_light {
                return {in_light.ka, in_light.kd, in_light.ks,
                        mul_point(dc.view_matrix, in_light.position)};
              });

    _frag_prg.set_uniform_block("scene_lighting", lights)
        .set_uniform("light_count", _lightcount)
        .set_uniform("mat_diffuse", 1)
        .set_uniform("mat_specular", 2)
        .set_uniform("mat_shininess", _mat_spec_pwr);

    gpu_program_pipeline_setup_builder{raw_handle(_prog_pipeline)}
        .add_vertex_program(_vertex_prg)
        .add_fragment_program(_frag_prg)
        .install();

    _obj_graphics.draw();
  }

  //
  // second pass, edge detection
  {
    //
    // set default framebuffer
    gl::BindFramebuffer(gl::FRAMEBUFFER, 0);

    gl::BindTextureUnit(0, raw_handle(_fbo.fbo_texture));
    gl::BindSampler(0, raw_handle(_sampler_obj));

    const auto surface_size = float2{static_cast<float>(dc.window_width),
                                     static_cast<float>(dc.window_height)};

    constexpr const char* const drawing_subroutines[] = {
        "color_default", "color_edges_sobel", "color_edges_frei_chen"};

    _fs_edge_detect.set_uniform("kSourceTexture", 0)
        .set_uniform("kEdgeTresholdSquared", _edge_treshold)
        .set_uniform("kSurfaceSize", surface_size)
        .set_subroutine_uniform("kDrawingStyle",
                                drawing_subroutines[_detect_method]);

    gpu_program_pipeline_setup_builder{raw_handle(_prog_pipeline)}
        .add_vertex_program(_vs_edge_detect)
        .add_fragment_program(_fs_edge_detect)
        .install();

    _fsquad_graphics.draw();
  }
}

void app::edge_detect_demo::update(const float /*delta_ms*/) {
  if (!_rotate_object)
    return;

  static constexpr auto ROTATION_SPEED_RADS = 0.01f;
  _rx += ROTATION_SPEED_RADS;
  if (_rx > two_pi<float>) {
    _rx -= two_pi<float>;
  }

  _ry += ROTATION_SPEED_RADS;
  if (_ry > two_pi<float>) {
    _ry -= two_pi<float>;
  }

  _rz += ROTATION_SPEED_RADS;
  if (_rz > two_pi<float>) {
    _rz -= two_pi<float>;
  }
}

void app::edge_detect_demo::key_event(const int32_t /*key_code*/,
                                      const int32_t /*action*/,
                                      const int32_t /*mods*/) {}

void app::edge_detect_demo::resize_event(const resize_context_t& resize_ctx) {
  assert(valid());

  const auto r_width  = static_cast<GLsizei>(resize_ctx.surface_width);
  const auto r_height = static_cast<GLsizei>(resize_ctx.surface_height);

  OUTPUT_DBG_MSG("Resize event [%u x %u]", resize_ctx.surface_width,
                 resize_ctx.surface_height);
  create_framebuffer(r_width, r_height);
}

void app::edge_detect_demo::create_framebuffer(const GLsizei r_width,
                                               const GLsizei r_height) {
  _fbo.fbo_depth   = scoped_renderbuffer{};
  _fbo.fbo_object  = scoped_framebuffer{};
  _fbo.fbo_texture = scoped_texture{};

  _fbo.fbo_texture = [r_width, r_height]() {
    GLuint tex{};
    gl::CreateTextures(gl::TEXTURE_2D, 1, &tex);
    gl::TextureStorage2D(tex, 1, gl::RGBA8, r_width, r_height);

    return tex;
  }();

  _fbo.fbo_depth = [r_width, r_height]() {
    GLuint depth_fb{};
    gl::CreateRenderbuffers(1, &depth_fb);
    gl::NamedRenderbufferStorage(depth_fb, gl::DEPTH_COMPONENT, r_width,
                                 r_height);
    return depth_fb;
  }();

  if (!_fbo.fbo_sampler) {
    _fbo.fbo_sampler = []() {
      GLuint smpl{};
      gl::CreateSamplers(1, &smpl);
      gl::SamplerParameteri(smpl, gl::TEXTURE_MIN_FILTER, gl::NEAREST);
      gl::SamplerParameteri(smpl, gl::TEXTURE_MAG_FILTER, gl::NEAREST);

      return smpl;
    }();
  }

  _fbo.fbo_object = [
    f_tex = raw_handle(_fbo.fbo_texture), f_depth = raw_handle(_fbo.fbo_depth)
  ]() {
    GLuint fbo{};
    gl::CreateFramebuffers(1, &fbo);
    gl::NamedFramebufferTexture(fbo, gl::COLOR_ATTACHMENT0, f_tex, 0);
    gl::NamedFramebufferRenderbuffer(fbo, gl::DEPTH_ATTACHMENT,
                                     gl::RENDERBUFFER, f_depth);
    gl::NamedFramebufferDrawBuffer(fbo, gl::COLOR_ATTACHMENT0);

    if (gl::CheckNamedFramebufferStatus(fbo, gl::DRAW_FRAMEBUFFER) !=
        gl::FRAMEBUFFER_COMPLETE) {
      XR_LOG_CRITICAL("Failed to create framebuffer!");
      XR_NOT_REACHED();
    }

    return fbo;
  }
  ();
}

void test_shit() {
  constexpr auto cfg_file = "config/cap6/edge_detect/app.new.conf";
  scene_loader   sldr{cfg_file};

  {
    const auto prg_desc = sldr.load_program_description("vs_phong");
    if (!prg_desc) {
      XR_LOG_ERR("Failed to load program description!");
      XR_NOT_REACHED();
    }

    auto vs = prg_desc.build<graphics_pipeline_stage::vertex>();
    if (vs) {
      XR_LOG_INFO("Uraaa, Stalin, Stalin !!!");
    }
  }

  {
    struct tmp_cls {
      void on_mtl_load(const mtl_desc_entry& e) {
        material_definitions.push_back(e);
      }

      fixed_vector<mtl_desc_entry, 8u> material_definitions;
    } tmp_obj;

    sldr.load_material_description(
        "p38", make_delegate(tmp_obj, &tmp_cls::on_mtl_load));

    auto mtl = sldr.create_material(&tmp_obj.material_definitions[0],
                                    tmp_obj.material_definitions.size());

    sldr.load_material_description(
        "copper", make_delegate(tmp_obj, &tmp_cls::on_mtl_load));

    for_each(begin(tmp_obj.material_definitions),
             end(tmp_obj.material_definitions), [](const auto& mdef) {
               XR_LOG_INFO("Material {}",
                           mtl_component_type::name(mdef.component_type));
             });
  }
}

void app::edge_detect_demo::init(const init_context_t& ini_ctx) {
  test_shit();

  const auto render_wnd_width  = static_cast<GLsizei>(ini_ctx.surface_width);
  const auto render_wnd_height = static_cast<GLsizei>(ini_ctx.surface_height);

  OUTPUT_DBG_MSG("Init window size %f x %f", render_wnd_width,
                 render_wnd_height);

  _sampler_obj = []() {
    GLuint smp_obj{};
    gl::CreateSamplers(1, &smp_obj);
    gl::SamplerParameteri(smp_obj, gl::TEXTURE_MIN_FILTER, gl::LINEAR);
    gl::SamplerParameteri(smp_obj, gl::TEXTURE_MAG_FILTER, gl::LINEAR);

    return smp_obj;
  }();

  create_framebuffer(render_wnd_width, render_wnd_height);

  {
    _vertex_prg = gpu_program_builder{}
                      .add_file("shaders/cap6/edge_detect/shader.vert")
                      .build<graphics_pipeline_stage::vertex>();

    _frag_prg = gpu_program_builder{}
                    .add_file("shaders/cap6/edge_detect/shader.frag")
                    .build<graphics_pipeline_stage::fragment>();
  }

  {
    _vs_edge_detect =
        gpu_program_builder{}
            .add_file("shaders/cap6/edge_detect/passthrough.vert.glsl")
            .build<graphics_pipeline_stage::vertex>();

    _fs_edge_detect =
        gpu_program_builder{}
            .add_file("shaders/cap6/edge_detect/edge_detect.frag.glsl")
            .build<graphics_pipeline_stage::fragment>();
  }

  if (!_vertex_prg || !_frag_prg || !_vs_edge_detect || !_fs_edge_detect) {
    XR_NOT_REACHED();
  }

  _prog_pipeline = scoped_program_pipeline_handle{[]() {
    GLuint id{};
    gl::CreateProgramPipelines(1, &id);
    return id;
  }()};

  config_file app_cfg{"config/cap6/edge_detect/app.conf"};
  if (!app_cfg) {
    XR_LOG_CRITICAL("Config file error {}", app_cfg.error());
    XR_NOT_REACHED();
  }

  {
    const char* model_file{nullptr};
    if (!app_cfg.lookup_value("app.scene.model", model_file)) {
      XR_LOG_CRITICAL("Failed to read model file name from config!");
      XR_NOT_REACHED();
    }

    _object = simple_mesh{vertex_format::pnt,
                          c_str_ptr(xr_app_config->model_path(model_file))};

    if (!_object) {
      XR_LOG_CRITICAL("Failed to load model from file!");
      XR_NOT_REACHED();
    }

    _obj_graphics = mesh_graphics_rep{_object};
    if (!_obj_graphics) {
      XR_LOG_CRITICAL("Failed to create mesh graphical representation!");
    }
  }

  {
    auto lights_entry = app_cfg.lookup_entry("app.scene.lights");
    if (!lights_entry) {
      XR_LOG_CRITICAL("Lights section not found in config file!");
      XR_NOT_REACHED();
    }

    _lightcount = xray::math::min<uint32_t>(lights_entry.length(),
                                            edge_detect_demo::max_lights);

    for (uint32_t idx = 0; idx < _lightcount; ++idx) {
      const auto le = lights_entry[idx];
      config_scene_reader::read_point_light(le, &_lights[idx]);
    }
  }

  _obj_material = [&app_cfg]() {
    const char* material_file{};
    app_cfg.lookup_value("app.scene.material.file", material_file);
    if (!material_file)
      return GLuint{};

    bool flip_yaxis{false};
    app_cfg.lookup_value("app.scene.material.flip_y", flip_yaxis);

    texture_loader tex_ldr{
        c_str_ptr(xr_app_config->texture_path(material_file)),
        flip_yaxis ? texture_load_options::flip_y : texture_load_options::none};

    if (!tex_ldr)
      return GLuint{};

    GLuint texh{};
    gl::CreateTextures(gl::TEXTURE_2D, 1, &texh);
    constexpr uint32_t internal_format[] = {gl::RGB8, gl::RGBA8};
    gl::TextureStorage2D(texh, 1, internal_format[tex_ldr.depth() == 4],
                         tex_ldr.width(), tex_ldr.height());
    constexpr uint32_t img_format[] = {gl::RGB, gl::RGBA};
    gl::TextureSubImage2D(texh, 0, 0, 0, tex_ldr.width(), tex_ldr.height(),
                          img_format[tex_ldr.depth() == 4], gl::UNSIGNED_BYTE,
                          tex_ldr.data());

    return texh;
  }();

  app_cfg.lookup_value("app.scene.shininess", _mat_spec_pwr);

  {
    geometry_data_t quad_geom{};
    geometry_factory::fullscreen_quad(&quad_geom);

    _fsquad          = simple_mesh{vertex_format::p, quad_geom};
    _fsquad_graphics = mesh_graphics_rep{_fsquad};
    //    assert(_fsquad_graphics && "Failed to create mesh GPU data!");
  }

  _valid = true;
}
