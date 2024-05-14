#include "xray/rendering/opengl/scoped_resource_mapping.hpp"

#include <cassert>

namespace xray::rendering {

ScopedResourceMapping::ScopedResourceMapping(ScopedResourceMapping&& rhs)
    : _map_addr(rhs._map_addr)
    , _mapping_length(rhs._mapping_length)
    , _resource_handle(rhs._resource_handle)
    , _access(rhs._access)
    , _mapping_offset(rhs._mapping_offset)
{
    std::memset(&rhs, 0, sizeof(rhs));
}

ScopedResourceMapping::ScopedResourceMapping(PrivateContructionToken tok,
                                             void* map_addr,
                                             uint64_t mapping_length,
                                             uint32_t resource_handle,
                                             uint32_t access,
                                             uint32_t mapping_offset)
    : _map_addr(map_addr)
    , _mapping_length(mapping_length)
    , _resource_handle(resource_handle)
    , _access(access)
    , _mapping_offset(mapping_length)
{
}

ScopedResourceMapping::~ScopedResourceMapping()
{
    if (_map_addr) {
        gl::FlushMappedNamedBufferRange(_resource_handle, _mapping_offset, _mapping_length);
        gl::UnmapNamedBuffer(_resource_handle);
    }
}

tl::optional<ScopedResourceMapping>
ScopedResourceMapping::create(const uint32_t resource,
                              const uint32_t access,
                              const uint32_t length,
                              const uint32_t offset)
{
    assert(resource != 0);
    const uint64_t maping_length{ [resource, length]() -> uint64_t {
        if (length != 0)
            return length;

        GLint64 buf_size{};
        gl::GetNamedBufferParameteri64v(resource, gl::BUFFER_SIZE, &buf_size);
        if (buf_size <= 0) {
            XR_LOG_ERR("Failed to query buffer object {:#x} size, error {:#x}", resource, gl::GetError());
            return 0u;
        }

        return static_cast<uint64_t>(buf_size);
    }() };

    void* map_addr = gl::MapNamedBufferRange(
        resource, offset, static_cast<GLsizei>(maping_length), access | gl::MAP_FLUSH_EXPLICIT_BIT);

    if (!map_addr) {
        XR_LOG_ERR("Failed to map resource {:#x}, access {:#x}, length {}, offset {}, error {:#x}",
                   resource,
                   access,
                   length,
                   offset,
                   gl::GetError());
        return tl::nullopt;
    }

    return tl::make_optional<ScopedResourceMapping>(
        PrivateContructionToken{}, map_addr, maping_length, resource, access | gl::MAP_FLUSH_EXPLICIT_BIT, offset);
}

} // namespace xray::rendering
