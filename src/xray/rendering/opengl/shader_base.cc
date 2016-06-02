#include "xray/rendering/opengl/shader_base.hpp"
#include "xray/base/debug/debug_ext.hpp"
#include "xray/base/logger.hpp"
#include <cassert>
#include <platformstl/filesystem/memory_mapped_file.hpp>
#include <stlsoft/memory/auto_buffer.hpp>

GLuint xray::rendering::make_shader(const uint32_t shader_type,
                                    const char * const* src_code_strings,
                                    const uint32_t strings_count) noexcept {
  assert(src_code_strings != nullptr);

  scoped_shader_handle tmp_handle{gl::CreateShader(shader_type)};
  if (!tmp_handle)
    return 0;

  using namespace xray::base;

  gl::ShaderSource(raw_handle(tmp_handle), strings_count, src_code_strings,
                   nullptr);
  gl::CompileShader(raw_handle(tmp_handle));

  //
  //  Get compile status
  {
    GLint compile_status{gl::FALSE_};
    gl::GetShaderiv(raw_handle(tmp_handle), gl::COMPILE_STATUS,
                    &compile_status);

    if (compile_status == gl::TRUE_)
      return unique_handle_release(tmp_handle);
  }

  //
  //  Get compile error
  {
    GLsizei log_length{};
    gl::GetShaderiv(raw_handle(tmp_handle), gl::INFO_LOG_LENGTH, &log_length);

    if (log_length > 0) {
      stlsoft::auto_buffer<GLchar, 1024> err_buff{
          static_cast<size_t>(log_length)};

      gl::GetShaderInfoLog(raw_handle(tmp_handle), log_length, nullptr,
                           err_buff.data());

      XR_LOG_ERR("Failed to compile shader, error {}", err_buff.data());
      //      OUTPUT_DBG_MSG("Failed to compile shader, error [%s]",
      //                     static_cast<const char *>(err_buff.data()));
    }
  }

  return 0;
}

GLuint xray::rendering::make_shader(const uint32_t shader_type,
                                    const char *source_file) noexcept {
  assert(source_file != nullptr);

  try {
    platformstl::memory_mapped_file shader_code_file{source_file};
    const char *src_code = static_cast<const char *>(shader_code_file.memory());

    return make_shader(shader_type, &src_code, 1u);
  } catch (const std::exception &ex) {
    XR_LOG_ERR("Failed to open shader file [{}], error [{}]", source_file,
               ex.what());
    //    OUTPUT_DBG_MSG("Failed to open shader file %s, error %s", source_file,
    //                   ex.what());
  }

  return 0;
}
