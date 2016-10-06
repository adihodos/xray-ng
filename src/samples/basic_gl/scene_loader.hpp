#pragma once

#include "xray/xray.hpp"
#include "mtl_component_type.hpp"
#include "xray/base/config_settings.hpp"
#include "xray/base/containers/fixed_stack.hpp"
#include "xray/base/fast_delegate.hpp"
#include "xray/math/math_std.hpp"
#include "xray/math/scalar4.hpp"
#include "xray/rendering/colors/rgb_color.hpp"
#include "xray/rendering/opengl/gl_handles.hpp"
#include "xray/rendering/opengl/gpu_program.hpp"
#include "xray/scene/point_light.hpp"
#include <platformstl/filesystem/path.hpp>

namespace app {

namespace detail {
template <typename U>
struct entry_as;

#define ENTRY_AS_SPECIALIZATION(type, fn)                                      \
  template <>                                                                  \
  struct entry_as<type> {                                                      \
    static type at(const xray::base::config_entry& e,                          \
                   const uint32_t                  idx) noexcept {             \
      return e.fn##_at(idx);                                                   \
    }                                                                          \
  }

ENTRY_AS_SPECIALIZATION(float, float);
ENTRY_AS_SPECIALIZATION(bool, bool);
ENTRY_AS_SPECIALIZATION(const char*, string);
ENTRY_AS_SPECIALIZATION(int32_t, int);

#undef ENTRY_AS_SPECIALIZATION

} // namespace detail

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

  static material create_material(const mtl_desc_entry* descriptions,
                                  const size_t          count);

  struct phong_material_parse_data {
    xray::base::fast_delegate<void(const mtl_desc_entry&)>
                                                 on_component_description;
    xray::base::fast_delegate<void(const float)> on_spec_power;
  };

  void parse_phong_material(const char*                      mtl_name,
                            const phong_material_parse_data& pdata);

  static xray::math::float4 read_float4(const xray::base::config_entry& e) {
    xray::math::float4 f4;
    read_array(e, 4, f4.components);
    return f4;
  }

  static xray::math::float3 read_float3(const xray::base::config_entry& e) {
    xray::math::float3 f3;
    read_array(e, 3, f3.components);
    return f3;
  }

  static xray::rendering::rgb_color
  read_color_f32(const xray::base::config_entry& e) {
    xray::rendering::rgb_color clr;
    if (read_array(e, 4, clr.components) == 3) {
      clr.a = 1.0f;
    }

    return clr;
  }

  static xray::rendering::rgb_color
  read_color_u32(const xray::base::config_entry& e) {
    int32_t comps[] = {0, 0, 0, 255};
    read_array(e, 4, comps);

    return xray::rendering::rgb_color{static_cast<float>(comps[0] / 255.0f),
                                      static_cast<float>(comps[1] / 255.0f),
                                      static_cast<float>(comps[2] / 255.0f),
                                      static_cast<float>(comps[3] / 255.0f)};
  }

  struct light_section_parse_data {
    xray::base::fast_delegate<void(const xray::scene::point_light& plight)>
                                      on_point_light_read;
    xray::base::fast_delegate<bool()> on_error;
  };

  void parse_point_lights(const light_section_parse_data& pdata);

private:
  void read_program_entry(const char*                           id,
                          xray::rendering::gpu_program_builder* prg_bld);

private:
  template <typename OutputIter>
  static uint32_t read_array(const xray::base::config_entry& entry,
                             const size_t max_items, OutputIter out_iter);

private:
  using path_stack_type =
      xray::base::fixed_stack<platformstl::basic_path<char>, 4>;

  static constexpr auto PRG_SEC_NAME          = "app.scene.programs";
  static constexpr auto MTL_SEC_NAME          = "app.scene.materials";
  static constexpr auto POINT_LIGHTS_SEC_NAME = "app.scene.point_lights";

  xray::base::config_file  _scene_file;
  xray::base::config_entry _prg_sec;
  path_stack_type          _prg_root_path;
  xray::base::config_entry _mtl_sec;
  xray::base::config_entry _sec_pt_lights;
};

template <typename OutputIter>
uint32_t scene_loader::read_array(const xray::base::config_entry& entry,
                                  const size_t max_items, OutputIter out_iter) {

  assert(entry && "Config entry must be valid!");

  const auto elements_count =
      xray::math::min<uint32_t>(max_items, entry.length());

  using output_type = typename std::remove_pointer<OutputIter>::type;

  for (uint32_t idx = 0; idx < elements_count; ++idx) {
    *out_iter++ = detail::entry_as<output_type>::at(entry, idx);
  }

  return elements_count;
}

} // namespace app