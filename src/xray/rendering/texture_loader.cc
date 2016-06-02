#include "xray/rendering/texture_loader.hpp"
#include "xray/base/logger.hpp"
#include <cassert>
#include <platformstl/filesystem/memory_mapped_file.hpp>

xray::rendering::texture_loader::texture_loader(
    const char* file_path, const texture_load_options load_opts) {
  assert(file_path != nullptr);

  try {
    platformstl::memory_mapped_file tex_file{file_path};

    if (load_opts == texture_load_options::flip_y)
      stbi_set_flip_vertically_on_load(true);

    xray::base::unique_pointer_reset(
        _texdata,
        stbi_load_from_memory(static_cast<const stbi_uc*>(tex_file.memory()),
                              static_cast<int32_t>(tex_file.size()), &_x_size,
                              &_y_size, &_levels, 0));
  } catch (const std::exception& e) {
    XR_LOG_ERR("Failed to load texture {}", file_path);
  }
}
