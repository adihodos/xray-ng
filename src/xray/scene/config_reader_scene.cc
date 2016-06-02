#include "xray/scene/config_reader_scene.hpp"
#include "xray/base/logger.hpp"
#include "xray/scene/config_reader_float3.hpp"
#include "xray/scene/config_reader_rgb_color.hpp"
#include "xray/scene/point_light.hpp"

using namespace xray::base;
using namespace xray::math;
using namespace xray::rendering;

bool xray::scene::config_scene_reader::read_point_light(
    const xray::base::config_entry& light_entry,
    xray::scene::point_light*       lsrc) noexcept {

  using reader = config_reader<float3_reader, rgb_color_reader>;

  {
    const auto pos_entry = light_entry.lookup("pos");
    if (!pos_entry) {
      XR_LOG_ERR("Fatal error : no position defined for light source !");
      return false;
    }

    if (!reader::read_float3(pos_entry, &lsrc->position)) {
      XR_LOG_ERR("Fatal error : failed to read position for point light!");
      return false;
    }
  }

  {
    const auto kd_entry = light_entry.lookup("kd");
    if (!kd_entry) {
      XR_LOG_ERR("Fatal error : no diffuse color defined for light source !");
      return false;
    }

    if (!reader::read_rgb_color(kd_entry, &lsrc->kd)) {
      XR_LOG_ERR("Fatal error : failed to read diffuse color !");
      return false;
    }
  }

  {
    const auto ka_entry = light_entry.lookup("ka");
    if (!ka_entry || !reader::read_rgb_color(ka_entry, &lsrc->ka))
      lsrc->ka = rgb_color{0.0f, 0.0f, 0.0f, 1.0f};
  }

  {
    const auto ks_entry = light_entry.lookup("ks");
    if (!ks_entry || !reader::read_rgb_color(ks_entry, &lsrc->ks))
      lsrc->ks = lsrc->kd;
  }

  return true;
}
