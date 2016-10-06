#include "scene_loader.hpp"
#include "xray/base/app_config.hpp"
#include "xray/base/scoped_guard.hpp"
#include "xray/base/shims/attribute/basic_path.hpp"
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
    , _sec_pt_lights{_scene_file.lookup_entry(POINT_LIGHTS_SEC_NAME)} {

  assert(_prg_sec);
  if (const auto top_root = _prg_sec.lookup_string("root_path")) {
    _prg_root_path.emplace(top_root.value());
  }
}

xray::rendering::gpu_program_builder
app::scene_loader::load_program_description(const char* id) {
  assert(id != nullptr);

  using namespace xray::rendering;

  gpu_program_builder prg_builder{};
  read_program_entry(id, &prg_builder);

  return prg_builder;
}

void app::scene_loader::read_program_entry(
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

void app::scene_loader::parse_phong_material(
    const char* mtl_name, const phong_material_parse_data& pdata) {

  assert(_mtl_sec && "No material section defined in config file!");
  assert(pdata.on_component_description && "Invalid parse event delegate!");
  assert(pdata.on_spec_power && "Invalid parse event delegate!");
  assert(mtl_name != nullptr);

  const auto msec = _mtl_sec.lookup(mtl_name);
  if (!msec) {
    XR_LOG_CRITICAL("Material {} not defined!", mtl_name);
    XR_NOT_REACHED();
  }

  using namespace std;

  auto mtl_def_parse_func = [&msec, mtl_name,
                             &pdata](const mtl_component_type::e mtl_type) {
    //
    //  lookup component definition (ambient, specular, etc)
    const auto def_sec = msec.lookup(mtl_component_type::name(mtl_type));

    if (!def_sec) {
      //
      //  At least Diffuse MUST be present
      if (mtl_type == mtl_component_type::e::diffuse) {
        XR_LOG_CRITICAL("Missing diffuse entry is mandatory for "
                        "material {} definition!",
                        mtl_name);
        XR_NOT_REACHED();
      }
      return;
    }

    //
    //  array -> RGB color
    if (def_sec.is_array()) {
      const auto clr = read_color_u32(def_sec);
      pdata.on_component_description(
          mtl_desc_entry{mtl_type, mtl_color_desc{clr}});
      return;
    }

    //
    // Texture file
    const auto fn = def_sec.lookup("file");
    const auto fy = def_sec.lookup("flip_y");

    pdata.on_component_description(
        mtl_desc_entry{mtl_type, mtl_file_desc{fn.as_string(), fy.as_bool()}});
  };

  for_each(mtl_component_type::cbegin(), mtl_component_type::cend(),
           mtl_def_parse_func);

  const auto spec_pow_entry = msec.lookup("spec_pwr");
  pdata.on_spec_power(spec_pow_entry ? spec_pow_entry.as_float() : 100.0f);
}

app::material
app::scene_loader::create_material(const mtl_desc_entry* descriptions,
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

void app::scene_loader::parse_point_lights(
    const light_section_parse_data& pdata) {

  if (!_sec_pt_lights)
    return;

  for (uint32_t i = 0, num_lights = _sec_pt_lights.length(); i < num_lights;
       ++i) {
    const auto&              plight_entry = _sec_pt_lights[i];
    xray::scene::point_light new_light;

    {
      const auto pos_entry = plight_entry.lookup("pos");
      if (!pos_entry) {
        XR_LOG_CRITICAL("Missing position attribute for light {}", i);
        XR_NOT_REACHED();
      }

      new_light.position = read_float3(pos_entry);
    }

    {
      const auto kd_entry = plight_entry.lookup("kd");
      if (!kd_entry) {
        XR_LOG_CRITICAL("Missing diffuse color attribute for light {}", i);
        XR_NOT_REACHED();
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

    pdata.on_point_light_read(new_light);
  }
}