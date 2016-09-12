#include "cap6/edge_detect/edge_detect_demo.hpp"
#include "helpers.hpp"
#include "std_assets.hpp"
#include "xray/base/app_config.hpp"
#include "xray/base/config_settings.hpp"
#include "xray/base/dbg/debug_ext.hpp"
#include "xray/base/fnv_hash.hpp"
#include "xray/base/logger.hpp"
#include "xray/base/shims/string.hpp"
#include "xray/math/constants.hpp"
#include "xray/math/math_std.hpp"
#include "xray/math/objects/aabb3.hpp"
#include "xray/math/objects/aabb3_math.hpp"
#include "xray/math/objects/sphere.hpp"
#include "xray/math/projection.hpp"
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
#include <imgui/imgui.h>
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

const app::basic_material*
app::material_cache::get_material(const char* mtl_name) const {
  const auto hashed_name = FNV::fnv1a(mtl_name);
  const auto cache_entry = _cached_materials.find(hashed_name);
  return cache_entry == std::end(_cached_materials) ? nullptr
                                                    : &cache_entry->second;
}

void app::material_cache::add_material(const char*                mtl_name,
                                       const app::basic_material& mtl) {
  const auto hashed_name = FNV::fnv1a(mtl_name);
  const auto cache_entry = _cached_materials.find(hashed_name);
  if (cache_entry != std::end(_cached_materials)) {
    XR_LOG_ERR("Material {} already cached!", mtl_name);
    return;
  }

  _cached_materials[hashed_name] = mtl;
}

app::texture_cache::texture_cache() {
  using namespace xray::rendering;

  _cached_textures[FNV::fnv1a("__null_pink_texture")] =
      make_color_texture(color_palette::web::pink, 1u, 1u);
  _cached_textures[FNV::fnv1a("__null_black_texture")] =
      make_color_texture(color_palette::web::black, 1u, 1u);
}

app::texture_cache::~texture_cache() {
  for (auto& name_tex_pair : _cached_textures)
    gl::DeleteTextures(1, &name_tex_pair.second);
}

app::texture_cache::texture_handle_type
app::texture_cache::make_color_texture(const xray::rendering::rgb_color& clr,
                                       const uint32_t                    width,
                                       const uint32_t height) noexcept {
  GLuint texh{};
  gl::CreateTextures(gl::TEXTURE_2D, 1, &texh);
  gl::TextureStorage2D(texh, 1, gl::RGBA8, static_cast<GLsizei>(width),
                       static_cast<GLsizei>(height));
  gl::TextureSubImage2D(texh, 0, 0, 0, static_cast<GLsizei>(width),
                        static_cast<GLsizei>(height), gl::RGBA, gl::FLOAT,
                        clr.components);

  return texh;
}

app::texture_cache::texture_handle_type app::texture_cache::pink_texture() const
    noexcept {
  const auto pink_tex_entry =
      _cached_textures.find(FNV::fnv1a("__null_pink_texture"));
  assert(pink_tex_entry != std::end(_cached_textures));
  return pink_tex_entry->second;
}

app::texture_cache::texture_handle_type
app::texture_cache::black_texture() const noexcept {
  const auto pink_tex_entry =
      _cached_textures.find(FNV::fnv1a("__null_black_texture"));
  assert(pink_tex_entry != std::end(_cached_textures));
  return pink_tex_entry->second;
}

app::texture_cache::texture_handle_type
app::texture_cache::get_texture(const char* name) noexcept {
  using namespace xray::base;

  const auto hashed_name = FNV::fnv1a(name);
  const auto tbl_entry   = _cached_textures.find(hashed_name);

  if (tbl_entry == std::end(_cached_textures)) {
    XR_LOG_ERR("Texture {} not in cache !", name);
    return pink_texture();
  }

  return tbl_entry->second;
}

app::texture_cache::texture_handle_type app::texture_cache::add_from_file(
    const char* name, const char* texture_path,
    xray::rendering::texture_load_options load_opts) {
  assert(texture_path != nullptr);
  const auto hashed_name    = FNV::fnv1a(name);
  auto       existing_entry = _cached_textures.find(hashed_name);

  if (existing_entry != std::end(_cached_textures)) {
    XR_LOG_ERR("Texture {} already cached!", name);
    return existing_entry->second;
  }

  using namespace xray::rendering;

  texture_loader tex_ldr{texture_path, load_opts};
  if (!tex_ldr) {
    return pink_texture();
  }

  GLuint texh{};
  gl::CreateTextures(gl::TEXTURE_2D, 1, &texh);
  gl::TextureStorage2D(texh, 1, tex_ldr.depth() == 4 ? gl::RGBA8 : gl::RGB8,
                       tex_ldr.width(), tex_ldr.height());
  gl::TextureSubImage2D(texh, 0, 0, 0, tex_ldr.width(), tex_ldr.height(),
                        tex_ldr.depth() == 4 ? gl::RGBA : gl::RGB,
                        gl::UNSIGNED_BYTE, tex_ldr.data());

  _cached_textures[hashed_name] = texh;
  return texh;
}

app::texture_cache::texture_handle_type
app::texture_cache::add_from_color(const char*                       name,
                                   const xray::rendering::rgb_color& color) {
  assert(name != nullptr);
  const auto hashed_name    = FNV::fnv1a(name);
  const auto existing_entry = _cached_textures.find(hashed_name);

  if (existing_entry != std::end(_cached_textures)) {
    XR_LOG_ERR("Material {} already cached!", name);
    return existing_entry->second;
  }

  const auto mtl_handle         = make_color_texture(color, 1u, 1u);
  _cached_textures[hashed_name] = mtl_handle;
  return mtl_handle;
}

const xray::rendering::simple_mesh*
app::mesh_cache::get_mesh(const char* name) const noexcept {
  const auto hashed_name  = FNV::fnv1a(name);
  const auto cached_entry = _cached_meshes.find(hashed_name);
  return cached_entry == std::end(_cached_meshes) ? nullptr
                                                  : &cached_entry->second;
}

const xray::rendering::simple_mesh*
app::mesh_cache::add_mesh(const char*                          name,
                          const xray::rendering::vertex_format fmt) {

  const auto hashed_name = FNV::fnv1a(name);
  assert((_cached_meshes.find(hashed_name) == std::end(_cached_meshes)) &&
         "Mesh already loaded into cache!");

  using namespace xray::rendering;
  auto loaded_mesh = simple_mesh{fmt, name};
  if (!loaded_mesh)
    return nullptr;

  _cached_meshes[hashed_name] = std::move(loaded_mesh);
  return &_cached_meshes[hashed_name];
}

const xray::rendering::simple_mesh*
app::mesh_cache::get_mesh(const app::model_id& id) const noexcept {
  const auto cached_entry = _stored_meshes.find(id);
  if (cached_entry == std::end(_stored_meshes)) {
    XR_LOG_ERR("Model id {} not found in cache !", id.hashed_name);
    return nullptr;
  }

  return &cached_entry->second;
}

const xray::rendering::simple_mesh*
app::mesh_cache::add_mesh(const app::model_id&         id,
                          xray::rendering::simple_mesh mesh) {

  assert(_stored_meshes.find(id) == std::end(_stored_meshes));
  _stored_meshes[id] = std::move(mesh);
  return &_stored_meshes[id];
}

void app::graphics_object::draw(
    const xray::rendering::draw_context_t& dc) noexcept {
  struct matrix_pack_t {
    float4x4 world_view_mtx;
    float4x4 normal_view_mtx;
    float4x4 world_view_proj_mtx;
  } const obj_transforms{dc.view_matrix * world_mtx, dc.view_matrix * world_mtx,
                         dc.proj_view_matrix * world_mtx};

  mtl->draw_prg->set_uniform_block("object_transforms", obj_transforms);

  {
    const GLuint textures[] = {mtl->ambient_map, mtl->diffuse_map,
                               mtl->specular_map};
    gl::BindTextures(0, XR_U32_COUNTOF__(textures), textures);
  }

  {
    const GLuint samplers[] = {mtl->sampler, mtl->sampler, mtl->sampler};
    gl::BindSamplers(0, XR_U32_COUNTOF__(samplers), samplers);
  }

  mtl->draw_prg->set_uniform("mtl_ambient", 0);
  mtl->draw_prg->set_uniform("mtl_diffuse", 1);
  mtl->draw_prg->set_uniform("mtl_specular", 2);
  mtl->draw_prg->set_uniform("mtl_spec_pwr", mtl->spec_pwr);
  mesh->draw();
}

xray::rendering::gpu_program*
app::shader_cache::add_program(const program_build_info& pbi) {

  const auto hashed_name = FNV::fnv1a(pbi.id);
  const auto cache_entry = _cached_programs.find(hashed_name);
  if (cache_entry != std::end(_cached_programs))
    return &cache_entry->second;

  using namespace xray::rendering;
  using namespace std;

  static constexpr GLuint NATIVE_SHADER_TYPES[] = {
      gl::VERTEX_SHADER, gl::TESS_CONTROL_SHADER, gl::TESS_EVALUATION_SHADER,
      gl::GEOMETRY_SHADER, gl::FRAGMENT_SHADER};

  GLuint compiled_shaders[5] = {0};
  transform(pbi.shaders_by_stage, pbi.shaders_by_stage + pbi.stage_count,
            begin(compiled_shaders),
            [ mapping = &NATIVE_SHADER_TYPES[0],
              rpath   = pbi.rootpath ](const auto& par) {

              const auto shader_file_path =
                  fmt::format("{}/{}", rpath, par.file_name);
              return make_shader(mapping[static_cast<uint32_t>(par.type)],
                                 raw_str(shader_file_path));
            });

  auto linked_prg = gpu_program{compiled_shaders, pbi.stage_count};
  if (!linked_prg) {
    XR_LOG_ERR("Failed to compile/link program {}", pbi.id);
    return nullptr;
  }

  _cached_programs[hashed_name] = std::move(linked_prg);
  return &_cached_programs[hashed_name];
}

xray::rendering::gpu_program*
app::shader_cache::get_program(const char* id) noexcept {
  assert(id != nullptr);

  const auto hashed_name = FNV::fnv1a(id);
  const auto cache_entry = _cached_programs.find(hashed_name);
  if (cache_entry != std::end(_cached_programs))
    return &cache_entry->second;

  XR_LOG_ERR("Shader {} not in cache !", id);
  return nullptr;
}

app::resource_store::~resource_store() {}

void app::resource_store::load_texture_materials(
    const xray::base::config_file& cfg, shader_cache* sstore,
    app::texture_cache* tex_cache, app::material_cache* mtl_cache) {

  const auto mtl_entry = cfg.lookup_entry("app.scene.materials");
  if (!mtl_entry) {
    XR_LOG_ERR("Error reading materials from file !");
    return;
  }

  for (uint32_t idx = 0, entries = mtl_entry.length(); idx < entries; ++idx) {
    const auto mtl = mtl_entry[idx];

    const auto mtl_id = mtl.lookup_string("id");
    if (!mtl_id) {
      XR_LOG_ERR("Id section not found for material {}", idx);
      continue;
    }

    app::basic_material m;
    m.ambient_map  = tex_cache->black_texture();
    m.diffuse_map  = tex_cache->pink_texture();
    m.specular_map = tex_cache->pink_texture();

    struct section_t {
      const char* name;
      uint32_t*   texid;
    } mtl_cfg_sections[] = {{"diffuse", &m.diffuse_map},
                            {"specular", &m.specular_map},
                            {"ambient", &m.ambient_map}};

    for (auto& sec : mtl_cfg_sections) {
      const auto sec_entry = mtl.lookup(sec.name);

      if (!sec_entry) {
        continue;
      }

      const auto mtl_name = sec_entry.lookup_string("file");
      if (!mtl_name) {
        continue;
      }

      const auto texture_full_path =
          xr_app_config->texture_path(value_of(mtl_name));
      const auto flip_img      = sec_entry.lookup_bool("flip_y");
      const auto img_load_opts = flip_img && value_of(flip_img)
                                     ? texture_load_options::flip_y
                                     : texture_load_options::none;

      *sec.texid = tex_cache->add_from_file(
          value_of(mtl_name), raw_str(texture_full_path), img_load_opts);
    }

    if (const auto spow = mtl.lookup_float("spec_pwr"))
      m.spec_pwr = value_of(spow);

    const auto shader_id = mtl.lookup_string("shader");
    if (!shader_id) {
      XR_LOG_CRITICAL("Material {} has no assigned shader!", value_of(mtl_id));
      XR_NOT_REACHED();
    }

    m.draw_prg = sstore->get_program(value_of(shader_id));
    if (!m.draw_prg) {
      XR_LOG_CRITICAL("Material {} required shader {} not found",
                      value_of(mtl_id), value_of(shader_id));
      XR_NOT_REACHED();
    }

    mtl_cache->add_material(value_of(mtl_id), m);
  }
}

void app::resource_store::load_color_materials(
    const xray::base::config_file& cfg, shader_cache* sstore,
    app::texture_cache* tex_cache, app::material_cache* mtl_cache) {

  const auto colors_sec = cfg.lookup_entry("app.scene.colors");
  if (!colors_sec)
    return;

  for (uint32_t idx = 0, entries = colors_sec.length(); idx < entries; ++idx) {
    const auto entry = colors_sec[idx];

    const auto          id = entry.lookup_string("id");
    app::basic_material m;
    m.ambient_map  = tex_cache->black_texture();
    m.diffuse_map  = tex_cache->pink_texture();
    m.specular_map = tex_cache->pink_texture();

    struct section_t {
      const char* name;
      uint32_t*   texhandle;
    } sections[] = {{"ka", &m.ambient_map},
                    {"kd", &m.diffuse_map},
                    {"ks", &m.specular_map}};

    for (auto& sec : sections) {
      const auto color_def = entry.lookup(sec.name);
      if (!color_def)
        continue;

      rgb_color mat_clr;
      if (!rgb_color_reader::read_rgb_color(color_def, &mat_clr))
        continue;

      char                        tmp_str[256];
      fmt::BasicArrayWriter<char> fmtbuff{tmp_str};
      fmtbuff.write("{}_{}", value_of(id), sec.name);
      *sec.texhandle = tex_cache->add_from_color(fmtbuff.c_str(), mat_clr);
    }

    if (const auto spow = entry.lookup_float("spec_pwr"))
      m.spec_pwr = value_of(spow);

    const auto shader_id = entry.lookup_string("shader");
    if (!shader_id) {
      XR_LOG_CRITICAL("Material {} has no assigned shader!", value_of(id));
      XR_NOT_REACHED();
    }

    m.draw_prg = sstore->get_program(value_of(shader_id));
    if (!m.draw_prg) {
      XR_LOG_CRITICAL("Material {} required shader {} not found", value_of(id),
                      value_of(shader_id));
      XR_NOT_REACHED();
    }

    mtl_cache->add_material(value_of(id), m);
  }
}

void app::resource_store::load_graphics_objects(
    const xray::base::config_file& cfg, const app::material_cache* mtl_cache,
    app::mesh_cache* meshes, std::vector<graphics_object>* objects) {

  XRAY_TIMED_FUNC();
  std::unordered_map<model_id, const char*> loadable_models;

  auto map_format = [](const char* fmt_desc) noexcept {
    if (!strcmp(fmt_desc, "pnt"))
      return vertex_format::pnt;

    if (!strcmp(fmt_desc, "pn"))
      return vertex_format::pn;

    return vertex_format::undefined;
  };

  {
    const auto section_models = cfg.lookup_entry("app.scene.models");
    if (!section_models) {
      XR_LOG_CRITICAL("No models section defined!");
      XR_NOT_REACHED();
    }

    for (uint32_t idx = 0; idx < section_models.length(); ++idx) {
      const auto entry = section_models[idx];

      const auto id = entry.lookup_string("id");
      assert(id && "Missing id entry!");
      const auto fmt = entry.lookup_string("format");
      assert(fmt && "Missing format entry!");
      const auto file_name = entry.lookup_string("file");
      assert(file_name && "Missing file name entry!");

      const auto mdl_id = model_id{value_of(id), map_format(value_of(fmt))};
      assert(loadable_models.find(mdl_id) == std::end(loadable_models));
      loadable_models[mdl_id] = value_of(file_name);
    }
  }

  unordered_map<model_id, const char*> models_to_load;

  {
    const auto objects_section = cfg.lookup_entry("app.scene.objects");
    if (!objects_section) {
      XR_LOG_CRITICAL("No objects section defined!");
      XR_NOT_REACHED();
    }

    objects->reserve(objects_section.length());
    for (uint32_t idx = 0, entries = objects_section.length(); idx < entries;
         ++idx) {
      const auto obj_def = objects_section[idx];

      const auto id = obj_def.lookup_string("id");
      assert(id && "Object id missing!");

      const auto mid = obj_def.lookup_string("model");
      assert(mid && "Objects missing model id !");
      const auto model_fmt = obj_def.lookup_string("format");
      assert(model_fmt && "Object missing model format!");

      const auto mdl_id =
          model_id{value_of(mid), map_format(value_of(model_fmt))};

      const auto loadable = loadable_models.find(mdl_id);
      if (loadable == end(loadable_models)) {
        XR_LOG_CRITICAL("Model {} is not in the list of loadable models",
                        value_of(mid));
        XR_NOT_REACHED();
      }

      models_to_load[mdl_id] = loadable->second;

      const auto mat_id = obj_def.lookup_string("material");
      assert(mat_id && "Object missing material id!");
    }
  }

  struct loadable_model {
    model_id    id;
    const char* file_name;
  };

  vector<loadable_model> model_load_list;
  transform(begin(models_to_load), end(models_to_load),
            back_inserter(model_load_list), [](const auto& kvp) {
              loadable_model lm;
              lm.id        = kvp.first;
              lm.file_name = kvp.second;
              return lm;
            });

  vector<geometry_data_t> loaded_geometry{model_load_list.size()};

  {
    XRAY_TIMED_SCOPE("parallel load models");

    using load_task_range_type = tbb::blocked_range<uint32_t>;

    tbb::parallel_for(
        load_task_range_type{0u, static_cast<uint32_t>(model_load_list.size())},
        [&model_load_list, &loaded_geometry](const auto& subrange) {

          for (uint32_t idx = subrange.begin(), range_end = subrange.end();
               idx != range_end; ++idx) {

            auto&      mdl       = model_load_list[idx];
            const auto file_path = xr_app_config->model_path(mdl.file_name);

            if (!geometry_factory::load_model(
                    &loaded_geometry[idx], file_path,
                    mesh_import_options::remove_points_lines)) {
              return;
            }
          }
        });
  }

  const auto any_failed =
      any_of(begin(loaded_geometry), end(loaded_geometry),
             [](const auto& geom) { return geom.vertex_count == 0; });

  if (any_failed) {
    XR_LOG_CRITICAL("Failed to load some models!");
    XR_NOT_REACHED();
  }

  const auto objects_section = cfg.lookup_entry("app.scene.objects");
  if (!objects_section) {
    XR_LOG_INFO("No objects section in config file!");
    return;
  }

  for (uint32_t idx = 0, cnt = model_load_list.size(); idx < cnt; ++idx) {
    const auto& mdl      = model_load_list[idx];
    const auto& geometry = loaded_geometry[idx];

    simple_mesh sm{mdl.id.format, geometry};
    if (!sm) {
      XR_LOG_CRITICAL("Failed to create mesh! {}", mdl.file_name);
      XR_NOT_REACHED();
    }

    meshes->add_mesh(mdl.id, move(sm));
  }

  //  const auto max_entries = objects_section.length();
  //  objects->resize(max_entries);

  //  struct graphic_object_load_info {
  //    const char*   model_name{};
  //    vertex_format vformat{vertex_format::undefined};
  //    bool          valid{false};
  //  };

  //  stlsoft::auto_buffer<graphic_object_load_info, 16> objects_to_load{
  //      max_entries};

  //  for (uint32_t idx = 0; idx < max_entries; ++idx) {
  //    auto& obj = objects_to_load[idx];

  //    const auto obj_entry = objects_section[idx];

  //    const auto name_entry = obj_entry.lookup_string("mesh");

  //    if (!name_entry) {
  //      XR_LOG_CRITICAL("Missing model name !!");
  //      XR_NOT_REACHED();
  //    }

  //    obj.model_name = value_of(name_entry);
  //  }

  //  for (uint32_t idx = 0; idx < max_entries; ++idx) {

  //    const auto obj_entry = objects_section[idx];
  //    const auto obj_id    = obj_entry.lookup_string("id");
  //    if (!obj_id) {
  //      XR_LOG_CRITICAL("Missing object id!");
  //      XR_NOT_REACHED();
  //    }

  //    const auto mesh_id = obj_entry.lookup_string("mesh");
  //    if (!mesh_id) {
  //      XR_LOG_CRITICAL("Missing mesh id !");
  //      XR_NOT_REACHED();
  //    }

  //    const auto mesh_path = xr_app_config->model_path(value_of(mesh_id));

  //    app::graphics_object graphic_obj;
  //    graphic_obj.hash = FNV::fnv1a(value_of(mesh_id));

  //    if (const auto mesh = meshes->get_mesh(raw_str(mesh_path))) {
  //      graphic_obj.mesh = mesh;
  //    } else {
  //      const auto fmt_desc = obj_entry.lookup_string("format");
  //      const auto fmt = [fstr = value_of(fmt_desc)]() {
  //        if (!strcmp(fstr, "pnt"))
  //          return vertex_format::pnt;

  //        if (!strcmp(fstr, "pn"))
  //          return vertex_format::pn;

  //        return vertex_format::undefined;
  //      }
  //      ();

  //      auto loaded_mesh = meshes->add_mesh(raw_str(mesh_path), fmt);
  //      if (!loaded_mesh) {
  //        XR_LOG_CRITICAL("Failed to load mesh {}", mesh_path);
  //        XR_NOT_REACHED();
  //      }

  //      graphic_obj.mesh = loaded_mesh;
  //    }

  //    const auto mat_id = obj_entry.lookup_string("material");
  //    assert(mat_id && "Missing material entry for object!");
  //    graphic_obj.mtl = mtl_cache->get_material(value_of(mat_id));

  //    objects->push_back(graphic_obj);
  //  }
}

void app::resource_store::load_shaders(const xray::base::config_file& cfg,
                                       app::shader_cache*             scache) {

  const auto shaders_sec = cfg.lookup_entry("app.scene.shaders");
  if (!shaders_sec) {
    XR_LOG_INFO("No shaders section in config file!");
    XR_NOT_REACHED();
  }

  const auto root_path = shaders_sec.lookup_string("root_path");
  if (!root_path) {
    XR_LOG_CRITICAL("No root path defined for shaders section!");
    XR_NOT_REACHED();
  }

  using namespace xray::base;
  const auto shader_list_sec = shaders_sec.lookup("entries");
  if (!shader_list_sec || shader_list_sec.length() == 0) {
    XR_LOG_CRITICAL("No shaders defined!");
    XR_NOT_REACHED();
  }

  for (uint32_t idx = 0, entries = shader_list_sec.length(); idx < entries;
       ++idx) {
    const auto prg_def = shader_list_sec[idx];

    const auto id = prg_def.lookup_string("id");

    struct shader_load_info {
      const char*      entry_name;
      app::shader_type type;
      bool             must_be_defined;
    } const entries_to_load[] = {
        {"vertex", app::shader_type::vertex, true},
        {"tess_control", app::shader_type::tess_control, false},
        {"tess_eval", app::shader_type::tess_eval, false},
        {"geometry", app::shader_type::geometry, false},
        {"fragment", app::shader_type::fragment, true}};

    app::program_build_info pbi;
    pbi.id       = value_of(id);
    pbi.rootpath = value_of(root_path);

    [
      prg_id = value_of(id), &prg_def, &pbi, fst_entry = begin(entries_to_load),
      lst_entry   = end(entries_to_load),
      output_info = begin(pbi.shaders_by_stage)
    ]() mutable {

      for (; fst_entry != lst_entry; ++fst_entry) {
        const auto file_name = prg_def.lookup_string(fst_entry->entry_name);
        if (!file_name) {
          if (fst_entry->must_be_defined) {
            XR_LOG_CRITICAL("Program id [{}] required entry {} is missing",
                            prg_id, fst_entry->entry_name);
            XR_NOT_REACHED();
          } else {
            continue;
          }
        }

        output_info->file_name = value_of(file_name);
        output_info->type      = fst_entry->type;

        ++output_info;
        ++pbi.stage_count;
      }
    }
    ();

    scache->add_program(pbi);
  }
}

void app::resource_store::load_graphics_objects(
    const xray::base::config_file& cfg_file,
    std::vector<graphics_object>*  objects) {

  load_shaders(cfg_file, this->shader_store);
  load_color_materials(cfg_file, this->shader_store, this->texture_store,
                       this->mat_store);
  load_texture_materials(cfg_file, this->shader_store, this->texture_store,
                         this->mat_store);
  load_graphics_objects(cfg_file, this->mat_store, this->mesh_store, objects);
}

app::edge_detect_demo::edge_detect_demo() { init(); }

app::edge_detect_demo::~edge_detect_demo() {}

void app::edge_detect_demo::compose_ui() {}

void app::edge_detect_demo::draw(const xray::rendering::draw_context_t& dc) {
  assert(valid());

  {
    //
    // first pass - normal phong lighting
    const auto obj_to_world = float4x4::stdc::identity;
    const auto obj_to_view  = dc.view_matrix * obj_to_world;

    struct matrix_pack_t {
      float4x4 world_view;
      float4x4 normal_view;
      float4x4 world_view_proj;
    } const obj_transforms{obj_to_view, obj_to_view,
                           dc.projection_matrix * obj_to_view};

    //    _drawprog_first_pass.set_uniform_block("transform_pack",
    //    obj_transforms);

    _vertex_prg.set_uniform_block("transform_pack", obj_transforms);

    point_light lights[edge_detect_demo::max_lights];
    transform(begin(_lights), end(_lights), begin(lights),
              [&dc](const auto& in_light) -> point_light {
                return {in_light.ka, in_light.kd, in_light.ks,
                        mul_point(dc.view_matrix, in_light.position)};
              });

    //    _drawprog_first_pass.set_uniform_block("scene_lighting", lights);
    _frag_prg.set_uniform_block("scene_lighting", lights);

    //    _drawprog_first_pass.set_uniform("light_count", _lightcount);
    _frag_prg.set_uniform("light_count", _lightcount);

    //    _drawprog_first_pass.set_uniform("mat_diffuse", 0);
    _frag_prg.set_uniform("mat_diffuse", 0);

    //    _drawprog_first_pass.set_uniform("mat_specular", 1);
    _frag_prg.set_uniform("mat_specular", 1);

    //    _drawprog_first_pass.set_uniform("mat_shininess", _mat_spec_pwr);
    _frag_prg.set_uniform("mat_shininess", _mat_spec_pwr);

    //    _drawprog_first_pass.bind_to_pipeline();

    _vertex_prg.use();
    _frag_prg.use();

    {
      const GLuint samplers[] = {raw_handle(_fbo.fbo_sampler),
                                 raw_handle(_fbo.fbo_sampler)};
      gl::BindSamplers(0, XR_U32_COUNTOF__(samplers), samplers);
    }

    {
      const GLuint materials[] = {raw_handle(_obj_material),
                                  raw_handle(_obj_material)};
      gl::BindTextures(0, XR_I32_COUNTOF__(materials), materials);
    }

    gpu_program_pipeline_setup_builder{raw_handle(_prog_pipeline)}
        .add_vertex_program(_vertex_prg)
        .add_fragment_program(_frag_prg)
        .install();

    _object.draw();
  }
}

void app::edge_detect_demo::update(const float /*delta_ms*/) {}

void app::edge_detect_demo::key_event(const int32_t /*key_code*/,
                                      const int32_t /*action*/,
                                      const int32_t /*mods*/) {}

void test_shit() {
  config_file app_cfg{"config/cap6/edge_detect/app.conf"};
  if (!app_cfg) {
    XR_LOG_ERR("Fatal error : config file not found !");
    return;
  }

  app::resource_store               rstore;
  std::vector<app::graphics_object> objs;
  rstore.load_graphics_objects(app_cfg, &objs);

  XR_LOG_INFO("Loaded {} graphics objects!", objs.size());
  XR_LOG_INFO("Done!");
}

void app::edge_detect_demo::init() {

  //  test_shit();

  uint32_t render_wnd_width{1024};
  uint32_t render_wnd_height{1024};

  _fbo.fbo_texture = [ w = render_wnd_width, h = render_wnd_height ]() {
    GLuint texh{};
    gl::CreateTextures(gl::TEXTURE_2D, 1, &texh);
    gl::TextureStorage2D(texh, 1, gl::RGBA8, w, h);

    return texh;
  }
  ();

  _fbo.fbo_sampler = []() {
    GLuint smpl{};
    gl::CreateSamplers(1, &smpl);
    gl::SamplerParameteri(smpl, gl::TEXTURE_MIN_FILTER, gl::LINEAR);
    gl::SamplerParameteri(smpl, gl::TEXTURE_MAG_FILTER, gl::LINEAR);

    return smpl;
  }();

  _fbo.fbo_depth = [ w = render_wnd_width, h = render_wnd_height ]() {
    GLuint depth_rbuff{};
    gl::CreateRenderbuffers(1, &depth_rbuff);
    gl::NamedRenderbufferStorage(depth_rbuff, gl::DEPTH_COMPONENT, w, h);

    return depth_rbuff;
  }
  ();

  _fbo.fbo_object = [
    tex = raw_handle(_fbo.fbo_texture), depth = raw_handle(_fbo.fbo_depth)
  ]() {
    GLuint fbo{};
    gl::CreateFramebuffers(1, &fbo);
    gl::NamedFramebufferTexture(fbo, gl::COLOR_ATTACHMENT0, tex, 0);
    gl::NamedFramebufferRenderbuffer(fbo, gl::DEPTH_ATTACHMENT,
                                     gl::RENDERBUFFER, depth);
    gl::NamedFramebufferDrawBuffer(fbo, gl::COLOR_ATTACHMENT0);

    const auto fbo_status =
        gl::CheckNamedFramebufferStatus(fbo, gl::DRAW_FRAMEBUFFER);

    if (fbo_status != gl::FRAMEBUFFER_COMPLETE) {
      XR_LOG_CRITICAL("Framebuffer is not complete!");
      XR_NOT_REACHED();
    }

    return fbo;
  }
  ();

  if (!_fbo.fbo_object) {
    XR_LOG_CRITICAL("Failed to create/init framebuffer!!");
    XR_NOT_REACHED();
  }

  //  _drawprog_first_pass = []() {
  //    const GLuint compiled_shaders[] = {
  //        make_shader(gl::VERTEX_SHADER,
  //        "shaders/cap6/edge_detect/shader.vert"),
  //        make_shader(gl::FRAGMENT_SHADER,
  //                    "shaders/cap6/edge_detect/shader.frag")};

  //    return gpu_program{compiled_shaders};
  //  }();

  _vertex_prg =
      vertex_program{gpu_program_builder{graphics_pipeline_stage::vertex}
                         .add_file("shaders/cap6/edge_detect/shader.vert")
                         .build()};

  _frag_prg =
      fragment_program{gpu_program_builder{graphics_pipeline_stage::fragment}
                           .add_file("shaders/cap6/edge_detect/shader.frag")
                           .build()};

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

    _object =
        simple_mesh{vertex_format::pnt, xr_app_config->model_path(model_file)};
    if (!_object) {
      XR_LOG_CRITICAL("Failed to load model from file!");
      XR_NOT_REACHED();
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

    texture_loader tex_ldr{xr_app_config->texture_path(material_file),
                           flip_yaxis ? texture_load_options::flip_y
                                      : texture_load_options::none};
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
  _valid = true;
}
