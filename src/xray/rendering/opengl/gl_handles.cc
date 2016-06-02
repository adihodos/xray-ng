#include "xray/rendering/opengl/gl_handles.hpp"

GLuint
xray::rendering::make_buffer(const uint32_t type, const uint32_t usage_flags,
                             const size_t size_in_bytes,
                             const void* initial_data /* = nullptr*/) noexcept {

  GLuint bhandle{};
  gl::GenBuffers(1, &bhandle);

  if (bhandle != 0) {
    gl::BindBuffer(type, bhandle);
    gl::BufferStorage(type, size_in_bytes, initial_data, usage_flags);
  }

  return bhandle;
}
