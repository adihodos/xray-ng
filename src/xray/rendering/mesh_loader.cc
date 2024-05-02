#include "xray/rendering/mesh_loader.hpp"
#include "xray/base/logger.hpp"
#include <system_error>

using namespace std;
using namespace xray::base;
using namespace xray::math;
using namespace xray::rendering;

tl::optional<mesh_loader>
xray::rendering::mesh_loader::load(const char* file_path,
                                   const uint32_t load_opts /*= mesh_load_option::remove_points_lines*/)
{
    std::error_code error;
    mio::mmap_source mmap = mio::make_mmap_source(file_path, error);
    if (error) {
        XR_LOG_ERR("Failed to load file {}, error {}", file_path, error.message());
        return tl::nullopt;
    }

    return tl::make_optional<mesh_loader>(std::move(mmap));
}

std::span<const xray::rendering::vertex_pnt>
xray::rendering::mesh_loader::vertex_span() const noexcept
{
    return { vertex_data(), vertex_data() + static_cast<size_t>(header().vertex_count) };
}

std::span<const uint32_t>
xray::rendering::mesh_loader::index_span() const noexcept
{
    return { index_data(), index_data() + static_cast<size_t>(header().index_count) };
}

const xray::rendering::vertex_pnt*
xray::rendering::mesh_loader::vertex_data() const noexcept
{
    const auto vstart =
        reinterpret_cast<const vertex_pnt*>(reinterpret_cast<const uint8_t*>(_mfile.data()) + header().vertex_offset);

    return vstart;
}

const uint32_t*
xray::rendering::mesh_loader::index_data() const noexcept
{
    const auto istart =
        reinterpret_cast<const uint32_t*>(reinterpret_cast<const uint8_t*>(_mfile.data()) + header().index_offset);

    return istart;
}

tl::optional<mesh_header>
xray::rendering::mesh_loader::read_header(const char* file_path)
{
    std::unique_ptr<FILE, void (*)(FILE*)> fp{ fopen(file_path, "rb"), [](FILE* f) {
                                                  if (f)
                                                      static_cast<void>(fclose(f));
                                              } };

    if (!fp) {
        XR_LOG_ERR("Failed to read header for file {}", file_path);
        return tl::nullopt;
    }

    mesh_header mhdr;
    const auto result{ (fread(&mhdr, sizeof(mhdr), 1, fp.get())) };
    static_cast<void>(result);

    return tl::optional{ mhdr };
}
