#include "scene_loader.hpp"
#include "xray/base/app_config.hpp"
#include "xray/base/scoped_guard.hpp"
#include "xray/base/shims/attribute/basic_path.hpp"
#include "xray/rendering/colors/color_palettes.hpp"
#include "xray/rendering/texture_loader.hpp"
#include <cassert>
#include <cstdint>
#include <opengl/opengl.hpp>

using namespace xray::base;
using namespace xray::rendering;
using namespace std;

extern xray::base::app_config* xr_app_config;

app::scene_loader::scene_loader(const char* scene_desc_file)
    : _scene_file{scene_desc_file}
    , _prg_sec{_scene_file.lookup_entry(PRG_SEC_NAME)}
    , _mtl_sec{_scene_file.lookup_entry(MTL_SEC_NAME)}
    , _sec_pt_lights{_scene_file.lookup_entry(POINT_LIGHTS_SEC_NAME)}
    , _sec_models{_scene_file.lookup_entry(MODEL_SEC_NAME)} {

  assert(_prg_sec);
  if (const auto top_root = _prg_sec.lookup_string("root_path")) {
    _prg_root_path.emplace(top_root.value());
  }
}

// xray::rendering::gpu_program_builder
// app::scene_loader::load_program_description(const char* id) {
//   assert(id != nullptr);

//   using namespace xray::rendering;

//   gpu_program_builder prg_builder{};
//   read_program_entry(id, &prg_builder);

//   return prg_builder;
// }

bool app::scene_loader::parse_program(
    const char* id, xray::rendering::gpu_program_builder* prg_bld) {

  assert(valid());
  assert(id != nullptr);
  assert(prg_bld != nullptr);

  const auto prg_entry = _prg_sec.lookup(id);
  if (!prg_entry) {
    XR_LOG_ERR("No program [[{}]] in program section", id);
    return false;
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

  return true;
}

bool app::scene_loader::parse_phong_material(
    const char* mtl_name, phong_material_builder* mtl_builder) {

  assert(_mtl_sec && "No material section defined in config file!");
  assert(mtl_name != nullptr);
  assert(mtl_builder != nullptr);

  const auto msec = _mtl_sec.lookup(mtl_name);
  if (!msec) {
    XR_LOG_INFO("Material {} not defined!", mtl_name);
    false;
  }

  using namespace std;

  auto check_if_required_and_not_present = [&msec, mtl_name, mtl_builder](
      const phong_material_component::e mtl_type) {
    //
    //  lookup component definition (ambient, specular, etc)
    const auto def_sec = msec.lookup(phong_material_component::name(mtl_type));

    if (!def_sec) {
      return mtl_type != phong_material_component::e::diffuse;
    }

    //
    //  array -> RGB color
    if (def_sec.is_array()) {
      const auto clr = read_color_u32(def_sec);
      mtl_builder->set_component_definition(phong_material_source_color{clr},
                                            mtl_type);
      return true;
    }

    //
    // Texture file
    const auto fn = def_sec.lookup("file");
    const auto fy = def_sec.lookup("flip_y");

    mtl_builder->set_component_definition(
        phong_material_source_texture{fn.as_string(), fy.as_bool()}, mtl_type);
    return true;
  };

  auto itr_fail = find_if_not(phong_material_component::cbegin(),
                              phong_material_component::cend(),
                              check_if_required_and_not_present);

  if (itr_fail != phong_material_component::cend()) {
    XR_LOG_ERR("Missing material section!");
    return false;
  }

  const auto spec_pow_entry = msec.lookup("spec_pwr");
  mtl_builder->set_specular_intensity(spec_pow_entry ? spec_pow_entry.as_float()
                                                     : 100.0f);

  return true;
}

// app::material
// app::scene_loader::create_material(const mtl_desc_entry* descriptions,
//                                    const size_t          count) {
//   material mtl;
//   auto tex_assign_fn = [&mtl](scoped_texture              tex_handle,
//                               const mtl_component_type::e comp_type) {
//     switch (comp_type) {
//     case mtl_component_type::e::ambient:
//       mtl.ambient = std::move(tex_handle);
//       break;

//     case mtl_component_type::e::diffuse:
//       mtl.diffuse = std::move(tex_handle);
//       break;

//     case mtl_component_type::e::emissive:
//       mtl.diffuse = std::move(tex_handle);
//       break;

//     case mtl_component_type::e::specular:
//       mtl.specular = std::move(tex_handle);
//       break;

//     default:
//       XR_LOG_CRITICAL("Unsupported component for material creation!");
//       XR_NOT_REACHED();
//       break;
//     }
//   };

//   for_each(descriptions, descriptions + count, [tex_assign_fn](
//                                                    const mtl_desc_entry&
//                                                        mtl_desc) {
//     scoped_texture texh{};
//     gl::CreateTextures(gl::TEXTURE_2D, 1, raw_handle_ptr(texh));

//     if (mtl_desc.source_type == mtl_entry_type::color) {
//       gl::TextureStorage2D(raw_handle(texh), 1, gl::RGBA8, 1, 1);
//       gl::TextureSubImage2D(raw_handle(texh), 0, 0, 0, 1, 1, gl::RGBA,
//                             gl::FLOAT, mtl_desc.clr.clr.components);
//       tex_assign_fn(std::move(texh), mtl_desc.component_type);
//       return;
//     }

//     texture_loader tex_ldr{
//         c_str_ptr(xr_app_config->texture_path(mtl_desc.file.path)),
//         mtl_desc.file.flip_y ? texture_load_options::flip_y
//                              : texture_load_options::none};

//     if (!tex_ldr) {
//       XR_NOT_REACHED();
//     }

//     static constexpr GLenum TEXTURE_INTERNAL_FORMAT[] = {gl::RGB8,
//     gl::RGBA8};

//     gl::TextureStorage2D(raw_handle(texh), 1,
//                          TEXTURE_INTERNAL_FORMAT[tex_ldr.depth() == 4],
//                          static_cast<GLsizei>(tex_ldr.width()),
//                          static_cast<GLsizei>(tex_ldr.height()));

//     static constexpr GLenum IMAGE_PIXEL_FORMAT[] = {gl::RGB, gl::RGBA};

//     gl::TextureSubImage2D(raw_handle(texh), 0, 0, 0,
//                           static_cast<GLsizei>(tex_ldr.width()),
//                           static_cast<GLsizei>(tex_ldr.height()),
//                           IMAGE_PIXEL_FORMAT[tex_ldr.depth() == 4],
//                           gl::UNSIGNED_BYTE, tex_ldr.data());

//     tex_assign_fn(std::move(texh), mtl_desc.component_type);
//   });

//   return mtl;
// }

uint32_t app::scene_loader::parse_point_lights(xray::scene::point_light* lights,
                                               const size_t lights_count) {
  assert(_sec_pt_lights && "No point lights section in config file!!");
  return parse_point_lights_section(_sec_pt_lights, lights, lights_count);
}

uint32_t app::scene_loader::parse_point_lights_section(
    const xray::base::config_entry& lights_sec,
    xray::scene::point_light* lights, const size_t lights_count) {

  assert(static_cast<bool>(lights_sec));
  assert(lights != nullptr);

  const auto light_cnt =
      xray::math::min<uint32_t>(lights_count, lights_sec.length());

  uint32_t lights_read{};
  for (uint32_t i = 0; i < light_cnt; ++i) {
    const auto& plight_entry = lights_sec[i];

    xray::scene::point_light new_light;

    {
      const auto pos_entry = plight_entry.lookup("pos");
      if (!pos_entry) {
        XR_LOG_INFO("Missing position attribute for light {}", i);
        continue;
      }

      new_light.position = read_vec3f(pos_entry);
    }

    {
      const auto kd_entry = plight_entry.lookup("kd");
      if (!kd_entry) {
        XR_LOG_INFO("Missing diffuse color attribute for light {}", i);
        continue;
      }

      new_light.kd = read_color_f32(kd_entry);
    }

    {
      const auto ka_entry = plight_entry.lookup("ka");
      new_light.ka        = ka_entry ? read_color_f32(ka_entry)
                              : rgb_color{0.0f, 0.0f, 0.0f, 1.0f};
    }

    {
      const auto ks_entry = plight_entry.lookup("ks");
      new_light.ks        = ks_entry ? read_color_f32(ks_entry)
                              : rgb_color{0.0f, 0.0f, 0.0f, 1.0f};
    }

    lights[lights_read++] = new_light;
  }

  return lights_read;
}

bool app::scene_loader::parse_mesh(const char*       mesh_name,
                                   mesh_create_info* mi) {

  assert(_sec_models && "No model section defined in configuration file!");
  assert(mesh_name != nullptr);
  assert(mi != nullptr);

  const auto mesh_sec_def = _sec_models.lookup(mesh_name);
  if (!mesh_sec_def) {
    XR_LOG_INFO("No model entry with id {}", mesh_name);
    return false;
  }

  const auto model_file = mesh_sec_def.lookup("file");
  if (!model_file) {
    XR_LOG_INFO("Model definition {} has no file entry!", mesh_name);
    return false;
  }

  const auto fmt_sec = mesh_sec_def.lookup("format");
  const auto fmt     = [](const char* fmt) {
    using namespace xray::rendering;

    if (!strcmp(fmt, "p"))
      return vertex_format::p;

    if (!strcmp(fmt, "pn"))
      return vertex_format::pn;

    if (!strcmp(fmt, "pnt"))
      return vertex_format::pnt;

    return vertex_format::pnt;
  }(fmt_sec.as_string());

  *mi = {model_file.as_string(), fmt};
  return true;
}

app::phong_material app::phong_material_builder::build() noexcept {
  assert(valid());

  auto make_texture_for_component_func =
      [](const phong_material_component_definition& cdef) {
        scoped_texture texture;
        gl::CreateTextures(gl::TEXTURE_2D, 1, raw_handle_ptr(texture));

        if (cdef.source_type == phong_material_source_type::color) {
          gl::TextureStorage2D(raw_handle(texture), 1, gl::RGBA8, 1, 1);
          gl::TextureSubImage2D(raw_handle(texture), 0, 0, 0, 1, 1, gl::RGBA,
                                gl::FLOAT, cdef.clr.clr.components);

          return texture;
        }

        const auto     texpath = xr_app_config->texture_path(cdef.file.path);
        texture_loader tex_ldr{c_str_ptr(texpath),
                               cdef.file.flip_y ? texture_load_options::flip_y
                                                : texture_load_options::none};

        if (!tex_ldr) {
          XR_LOG_CRITICAL("Failed to load texture {} !", c_str_ptr(texpath));
          XR_NOT_REACHED();
        }

        constexpr GLenum TEXTURE_INTERNAL_FMT[] = {gl::RGB8, gl::RGBA8};
        constexpr GLenum TEXTURE_IMAGE_FORMAT[] = {gl::RGB, gl::RGBA};

        gl::TextureStorage2D(raw_handle(texture), 1,
                             TEXTURE_INTERNAL_FMT[tex_ldr.depth() == 4],
                             tex_ldr.width(), tex_ldr.height());
        gl::TextureSubImage2D(raw_handle(texture), 0, 0, 0, tex_ldr.width(),
                              tex_ldr.height(),
                              TEXTURE_IMAGE_FORMAT[tex_ldr.depth() == 4],
                              gl::UNSIGNED_BYTE, tex_ldr.data());

        return texture;
      };

  phong_material phong_mtl;
  phong_mtl.spec_pwr = _spec_pwr;

  struct {
    phong_material_component::e component;
    scoped_texture*             output_texture;
  } per_component_outputs[] = {
      {phong_material_component::e::emissive, &phong_mtl.emissive},
      {phong_material_component::e::ambient, &phong_mtl.ambient},
      {phong_material_component::e::diffuse, &phong_mtl.diffuse},
      {phong_material_component::e::specular, &phong_mtl.specular}};

  // for_each(
  //     begin(per_component_outputs), end(per_component_outputs),
  //     [make_texture_for_component_func, this, &phong_mtl](const auto&
  //     cmpdata) {
  //       if (!has_component(cmpdata.component))
  //         return;

  //       const auto& comp_def    = component_def(cmpdata.component);
  //       *cmpdata.output_texture = make_texture_for_component_func(comp_def);
  //     });

  return phong_mtl;
}