#pragma once

#include "xray/xray.hpp"
#include "phong_material_component.hpp"
#include "xray/base/config_settings.hpp"
#include "xray/base/containers/fixed_stack.hpp"
#include "xray/base/fast_delegate.hpp"
#include "xray/math/math_std.hpp"
#include "xray/math/scalar4.hpp"
#include "xray/rendering/colors/rgb_color.hpp"
#include "xray/rendering/opengl/gl_handles.hpp"
#include "xray/rendering/opengl/gpu_program.hpp"
#include "xray/rendering/vertex_format/vertex_format.hpp"
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

struct phong_material_source_texture {
  const char* path;
  bool        flip_y;
};

struct phong_material_source_color {
  xray::rendering::rgb_color clr;
};

enum class phong_material_source_type { tex, color };

struct phong_material_component_definition {
  phong_material_source_type source_type;

  union {
    phong_material_source_texture file;
    phong_material_source_color   clr;
  };

  phong_material_component_definition() = default;

  phong_material_component_definition(const phong_material_source_texture& fd)
      : source_type{phong_material_source_type::tex}, file{fd} {}

  phong_material_component_definition(const phong_material_source_color& clr_)
      : source_type{phong_material_source_type::color}, clr{clr_} {}
};

struct phong_material {
  xray::rendering::scoped_texture diffuse;
  xray::rendering::scoped_texture specular;
  xray::rendering::scoped_texture emissive;
  xray::rendering::scoped_texture ambient;
  float                           spec_pwr{100.0f};

  phong_material() = default;
  XRAY_DEFAULT_MOVE(phong_material);

  bool valid() const noexcept { return static_cast<bool>(diffuse); }

  explicit operator bool() const noexcept { return valid(); }

private:
  XRAY_NO_COPY(phong_material);
};

struct phong_material_builder {
public:
  phong_material_builder() noexcept = default;

  phong_material_builder&
  set_emissive(const phong_material_component_definition& def) noexcept {
    set_component_definition(def, phong_material_component::e::emissive);
    return *this;
  }

  phong_material_builder&
  set_ambient(const phong_material_component_definition& def) noexcept {
    set_component_definition(def, phong_material_component::e::ambient);
    return *this;
  }

  phong_material_builder&
  set_diffuse(const phong_material_component_definition& def) noexcept {
    set_component_definition(def, phong_material_component::e::diffuse);
    return *this;
  }

  phong_material_builder&
  set_specular(const phong_material_component_definition& def) noexcept {
    set_component_definition(def, phong_material_component::e::specular);
    return *this;
  }

  phong_material_builder&
  set_specular_intensity(const float spec_intensity) noexcept {
    _spec_pwr = spec_intensity;
    return *this;
  }

  void
  set_component_definition(const phong_material_component_definition& cdef,
                           const phong_material_component::e ctype) noexcept {
    _cdefs[phong_material_component::to_integer(
        phong_material_component::e::emissive)] = cdef;
    _cbits |= 1u << phong_material_component::to_integer(
                  phong_material_component::e::emissive);
  }

  phong_material build() noexcept;

  explicit operator bool() const noexcept { return valid(); }

  bool valid() const noexcept {
    return has_component(phong_material_component::e::diffuse);
  }

private:
  const phong_material_component_definition&
  component_def(const phong_material_component::e cmp) noexcept {
    assert(has_component(cmp));
    return _cdefs[phong_material_component::to_integer(cmp)];
  }

  bool has_component(const phong_material_component::e cmp_type) const
      noexcept {
    return (_cbits & (1 << phong_material_component::to_integer(cmp_type))) !=
           0;
  }

private:
  phong_material_component_definition
           _cdefs[phong_material_component::size - 1];
  uint32_t _cbits{0};
  float    _spec_pwr{0.0f};
};

struct mesh_create_info {
  union {
    struct {
      const char* file_name;
    };
  };

  xray::rendering::vertex_format format{xray::rendering::vertex_format::pnt};

  mesh_create_info() = default;
  mesh_create_info(const char*                          fname,
                   const xray::rendering::vertex_format fmt) noexcept
      : file_name{fname}, format{fmt} {}
};

class scene_loader {
public:
  explicit scene_loader(const char* scene_def_file);

  explicit operator bool() const noexcept { return valid(); }
  bool valid() const noexcept { return _scene_file.valid(); }

  bool parse_phong_material(const char*             mtl_name,
                            phong_material_builder* mtl_builder);

  bool parse_mesh(const char* mesh_name, mesh_create_info* mi);

  bool parse_program(const char*                           id,
                     xray::rendering::gpu_program_builder* prg_bld);

  uint32_t parse_point_lights(xray::scene::point_light* lighs,
                              const size_t              lights_count);

  static uint32_t
  parse_point_lights_section(const xray::base::config_entry& lights_sec,
                             xray::scene::point_light*       lighs,
                             const size_t                    lights_count);

public:
  static xray::math::vec4f read_vec4f(const xray::base::config_entry& e) {
    xray::math::vec4f f4;
    read_array(e, 4, f4.components);
    return f4;
  }

  static xray::math::vec3f read_vec3f(const xray::base::config_entry& e) {
    xray::math::vec3f f3;
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

private:
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
  static constexpr auto MODEL_SEC_NAME        = "app.scene.models";

  xray::base::config_file  _scene_file;
  xray::base::config_entry _prg_sec;
  path_stack_type          _prg_root_path;
  xray::base::config_entry _mtl_sec;
  xray::base::config_entry _sec_pt_lights;
  xray::base::config_entry _sec_models;
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