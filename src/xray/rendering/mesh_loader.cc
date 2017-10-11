#include "xray/rendering/mesh_loader.hpp"
#include "xray/base/logger.hpp"

using namespace std;
using namespace xray::base;
using namespace xray::math;
using namespace xray::rendering;

bool xray::rendering::mesh_loader::load(
  const char*    file_path,
  const uint32_t load_opts /*= mesh_load_option::remove_points_lines*/) {
  try {
    _mfile = platformstl::memory_mapped_file{file_path};
  } catch (const stlsoft::os_exception& e) {
    XR_LOG_ERR("Failed to load file {}, error {}", file_path, e.what());
    return false;
  }

  return true;
}

gsl::span<const xray::rendering::vertex_pnt>
xray::rendering::mesh_loader::vertex_span() const noexcept {

  return {vertex_data(),
          vertex_data() + static_cast<ptrdiff_t>(header().vertex_count)};
}

gsl::span<const uint32_t> xray::rendering::mesh_loader::index_span() const
  noexcept {
  return {index_data(),
          index_data() + static_cast<ptrdiff_t>(header().index_count)};
}

const xray::rendering::vertex_pnt*
xray::rendering::mesh_loader::vertex_data() const noexcept {
  assert(valid());
  const auto vstart = reinterpret_cast<const vertex_pnt*>(
    reinterpret_cast<const uint8_t*>(_mfile.memory()) + header().vertex_offset);

  return vstart;
}

const uint32_t* xray::rendering::mesh_loader::index_data() const noexcept {
  assert(valid());

  const auto istart = reinterpret_cast<const uint32_t*>(
    reinterpret_cast<const uint8_t*>(_mfile.memory()) + header().index_offset);

  return istart;
}

xray::base::maybe<mesh_header>
xray::rendering::mesh_loader::read_header(const char* file_path) {
  std::unique_ptr<FILE, decltype(&fclose)> fp{fopen(file_path, "rb"), &fclose};
  if (!fp) {
    XR_LOG_ERR("Failed to read header for file {}", file_path);
    return xray::base::nothing{};
  }

  mesh_header mhdr;
  fread(&mhdr, sizeof(mhdr), 1, fp.get());

  return mhdr;
}
