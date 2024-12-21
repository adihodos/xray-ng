#include "xray/rendering/texture_loader.hpp"
#include "xray/base/logger.hpp"
#include <cassert>
#include <mio/mmap.hpp>
#include <system_error>

#include <stb/stb_image.h>

void
xray::rendering::detail::stbi_img_deleter::operator()(uint8_t* mem) const noexcept
{
    if (mem) {
        stbi_image_free(mem);
    }
}

xray::rendering::texture_loader::~texture_loader() {}

xray::rendering::texture_loader::texture_loader(const std::filesystem::path& file_path,
                                                const texture_load_options load_opts)
{
    std::error_code err_code{};
    const mio::mmap_source tex_file{ mio::make_mmap_source(file_path.generic_string(), err_code) };
    if (err_code) {
        XR_LOG_ERR("Failed to open texture file: {}, error {:#x} - {}",
                   file_path.generic_string(),
                   err_code.value(),
                   err_code.message());
        return;
    }

    if (load_opts == texture_load_options::flip_y)
        stbi_set_flip_vertically_on_load(true);

    xray::base::unique_pointer_reset(_texdata,
                                     stbi_load_from_memory(reinterpret_cast<const stbi_uc*>(tex_file.data()),
                                                           static_cast<int32_t>(tex_file.size()),
                                                           &_x_size,
                                                           &_y_size,
                                                           &_levels,
                                                           0));
}

GLenum
xray::rendering::texture_loader::format() const noexcept
{
    assert(_texdata != nullptr);

    switch (_levels) {
        case 1:
            return gl::RED;
            break;

        case 2:
            return gl::RG;
            break;

        case 3:
            return gl::RGB;
            break;

        case 4:
            return gl::RGBA;
            break;

        default:
            break;
    }

    return gl::INVALID_ENUM;
}

GLenum
xray::rendering::texture_loader::internal_format() const noexcept
{
    assert(_texdata != nullptr);

    switch (_levels) {
        case 1:
            return gl::R8;
            break;

        case 2:
            return gl::RG8;
            break;

        case 3:
            return gl::RGB8;
            break;

        case 4:
            return gl::RGBA8;
            break;

        default:
            break;
    }

    return gl::INVALID_ENUM;
}
