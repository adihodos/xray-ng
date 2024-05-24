#include "xray/rendering/opengl/shader_base.hpp"
#include "xray/base/logger.hpp"
#include <cassert>
#include <itlib/small_vector.hpp>
#include <mio/mmap.hpp>
#include <system_error>

GLuint
xray::rendering::make_shader(const uint32_t shader_type,
                             const char* const* src_code_strings,
                             const uint32_t strings_count) noexcept
{
    assert(src_code_strings != nullptr);

    scoped_shader_handle tmp_handle{ gl::CreateShader(shader_type) };
    if (!tmp_handle)
        return 0;

    using namespace xray::base;

    gl::ShaderSource(raw_handle(tmp_handle), strings_count, src_code_strings, nullptr);
    gl::CompileShader(raw_handle(tmp_handle));

    //
    //  Get compile status
    {
        GLint compile_status{ gl::FALSE_ };
        gl::GetShaderiv(raw_handle(tmp_handle), gl::COMPILE_STATUS, &compile_status);

        if (compile_status == gl::TRUE_)
            return unique_handle_release(tmp_handle);
    }

    //
    //  Get compile error
    {
        GLsizei log_length{};
        gl::GetShaderiv(raw_handle(tmp_handle), gl::INFO_LOG_LENGTH, &log_length);

        if (log_length > 0) {
            itlib::small_vector<GLchar, 2048> err_buff;
            gl::GetShaderInfoLog(raw_handle(tmp_handle), log_length, nullptr, err_buff.data());
            err_buff.push_back(0);

            XR_LOG_ERR("Failed to compile shader, error {}", err_buff.data());
        }
    }

    return 0;
}

GLuint
xray::rendering::make_shader(const uint32_t shader_type, const char* source_file) noexcept
{
    assert(source_file != nullptr);

    std::error_code err_code{};
    const mio::mmap_source shader_code_file{ mio::make_mmap_source(source_file, err_code) };
    if (err_code) {
        XR_LOG_ERR(
            "Failed to open shader file [{}], error [{:#x} - {}]", source_file, err_code.value(), err_code.message());
        return 0;
    }

    const char* src_code = reinterpret_cast<const char*>(shader_code_file.data());
    return make_shader(shader_type, &src_code, 1u);
}
