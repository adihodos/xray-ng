#include "xray/rendering/mesh.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <unordered_map>
#include <vector>

#include <oneapi/tbb/blocked_range.h>
#include <oneapi/tbb/parallel_for.h>
#include <oneapi/tbb/parallel_for_each.h>
#include <oneapi/tbb/parallel_reduce.h>

#include <Lz/Lz.hpp>
#include <itlib/small_vector.hpp>

#include "xray/base/basic_timer.hpp"
#include "xray/base/logger.hpp"
#include "xray/math/math_std.hpp"
#include "xray/math/objects/aabb3_math.hpp"
#include "xray/math/objects/sphere_math.hpp"
#include "xray/math/scalar2.hpp"
#include "xray/math/scalar3.hpp"
#include "xray/math/scalar3_math.hpp"
#include "xray/math/scalar4x4.hpp"
#include "xray/math/scalar4x4_math.hpp"
#include "xray/math/transforms_r4.hpp"
#include "xray/rendering/mesh_loader.hpp"
#include "xray/rendering/opengl/scoped_resource_mapping.hpp"
#include "xray/rendering/vertex_format/vertex_pnt.hpp"

using namespace std;
using namespace xray::base;
using namespace xray::math;
using namespace xray::rendering;

xray::rendering::basic_mesh::basic_mesh(const std::filesystem::path& path, const mesh_type type)
    : _mtype{ type }
{
    using namespace xray::rendering;

    tl::optional<mesh_loader> mloader{ mesh_loader::load(path) };
    if (!mloader) {
        XR_LOG_CRITICAL("Failed to load mesh {}", path.generic_string());
        return;
    }

    mloader.map([this](const mesh_loader& loaded_mdl) {
        _vertices.resize(loaded_mdl.vertex_span().size());
        _indices.resize(loaded_mdl.index_span().size());

        std::copy(std::cbegin(loaded_mdl.vertex_span()), std::cend(loaded_mdl.vertex_span()), std::begin(_vertices));
        std::copy(std::cbegin(loaded_mdl.index_span()), std::cend(loaded_mdl.index_span()), std::begin(_indices));

        _aabb = loaded_mdl.bounding().axis_aligned_bbox;
        _bsphere = loaded_mdl.bounding().bounding_sphere;
    });

    if (const vec3f origin = _aabb.center(); !is_zero_length(origin)) {
        //
        // correct off-center model
        namespace mt = oneapi::tbb;
        mt::parallel_for(mt::blocked_range<vertex_pnt*>{ _vertices.data(), _vertices.data() + _vertices.size() },
                         [origin](mt::blocked_range<vertex_pnt*> r) {
                             for (vertex_pnt* v = r.begin(); v != r.end(); ++v) {
                                 v->position -= origin;
                             }
                         });

        _aabb.min -= origin;
        _aabb.max -= origin;
        _bsphere.center = vec3f::stdc::zero;
    }

    setup_buffers();
}

xray::rendering::basic_mesh::basic_mesh(const xray::rendering::vertex_pnt* vertices,
                                        const size_t num_vertices,
                                        const uint32_t* indices,
                                        const size_t num_indices,
                                        const mesh_type mtype)
    : _mtype{ mtype }
{
    create(std::span<const vertex_pnt>{ vertices, vertices + static_cast<size_t>(num_vertices) },
           std::span<const uint32_t>{ indices, indices + static_cast<size_t>(num_indices) });
    compute_bounding();
}

void
xray::rendering::basic_mesh::create(const std::span<const vertex_pnt> vertices, const std::span<const uint32_t> indices)
{
    assert(!vertices.empty());
    assert(!indices.empty());

    _vertices.resize(vertices.size());
    _indices.resize(indices.size());

    memcpy(_vertices.data(), vertices.data(), vertices.size() * sizeof(vertices[0]));
    memcpy(_indices.data(), indices.data(), indices.size() * sizeof(indices[0]));

    setup_buffers();
}

void
xray::rendering::basic_mesh::compute_bounding()
{
    assert(!_vertices.empty());

    timer_highp op_tm;

    static constexpr size_t PARALLEL_REDUCE_MIN_VERTEX_COUNT = 120'000'000u;

    namespace mt = oneapi::tbb;

    if (_vertices.size() >= PARALLEL_REDUCE_MIN_VERTEX_COUNT) {
        scoped_timing_object<timer_highp> sto{ &op_tm };

        _aabb = mt::parallel_reduce(
            mt::blocked_range{ static_cast<const vertex_pnt*>(_vertices.data()),
                               static_cast<const vertex_pnt*>(_vertices.data()) + _vertices.size() },
            aabb3f::stdc::identity,
            [](const tbb::blocked_range<const vertex_pnt*>& rng, const aabb3f& initial_bbox) {
                aabb3f box{ initial_bbox };
                for (const vertex_pnt* b = rng.begin(); b != rng.end(); ++b) {
                    box.max = max(box.max, b->position);
                    box.min = min(box.min, b->position);
                }

                return box;
            },
            [](const aabb3f& a, const aabb3f& b) { return math::merge(a, b); });
    } else {
        scoped_timing_object<timer_highp> sto{ &op_tm };

        for_each(begin(_vertices), end(_vertices), [this](const vertex_pnt& v) {
            _aabb.min = math::min(_aabb.min, v.position);
            _aabb.max = math::max(_aabb.max, v.position);
        });
    }

    if (const vec3f origin = _aabb.center(); !is_zero_length(origin)) {
        //
        // correct off-center model
        mt::parallel_for(mt::blocked_range<vertex_pnt*>{ _vertices.data(), _vertices.data() + _vertices.size() },
                         [origin](mt::blocked_range<vertex_pnt*> r) {
                             for (vertex_pnt* v = r.begin(); v != r.end(); ++v) {
                                 v->position -= origin;
                             }
                         });

        _aabb = math::transform(R4::translate(-origin), _aabb);
    }

    XR_LOG_INFO("AABB : [{:3.3f}, {:3.3f}, {:3.3f}] : [{:3.3f}, "
                "{:3.3f}, {:3.3f}]\n Time {:3.3f}",
                _aabb.min.x,
                _aabb.min.y,
                _aabb.min.z,
                _aabb.max.x,
                _aabb.max.y,
                _aabb.max.z,
                op_tm.elapsed_millis());

    _bsphere = xray::math::bounding_sphere<float>(
        begin(_vertices), end(_vertices), [](const vertex_pnt& vs_in) { return vs_in.position; });
}

void
xray::rendering::basic_mesh::setup_buffers()
{
    gl::CreateBuffers(1, raw_handle_ptr(_vertexbuffer));
    gl::NamedBufferStorage(raw_handle(_vertexbuffer),
                           xray::base::container_bytes_size(_vertices),
                           _vertices.data(),
                           _mtype == mesh_type::readonly ? GLbitfield{ 0 } : gl::MAP_WRITE_BIT);

    gl::CreateBuffers(1, raw_handle_ptr(_indexbuffer));
    gl::NamedBufferStorage(raw_handle(_indexbuffer),
                           xray::base::container_bytes_size(_indices),
                           _indices.data(),
                           _mtype == mesh_type::readonly ? GLbitfield{ 0 } : gl::MAP_WRITE_BIT);

    gl::CreateVertexArrays(1, raw_handle_ptr(_vertexarray));
    const auto vao = raw_handle(_vertexarray);

    gl::VertexArrayVertexBuffer(vao, 0, raw_handle(_vertexbuffer), 0, sizeof(vertex_pnt));
    gl::VertexArrayElementBuffer(vao, raw_handle(_indexbuffer));

    gl::EnableVertexArrayAttrib(vao, 0);
    gl::VertexArrayAttribFormat(vao, 0, 3, gl::FLOAT, gl::FALSE_, XR_U32_OFFSETOF(vertex_pnt, position));
    gl::VertexArrayAttribBinding(vao, 0, 0);

    gl::EnableVertexArrayAttrib(vao, 1);
    gl::VertexArrayAttribFormat(vao, 1, 3, gl::FLOAT, gl::FALSE_, XR_U32_OFFSETOF(vertex_pnt, normal));
    gl::VertexArrayAttribBinding(vao, 1, 0);

    gl::EnableVertexArrayAttrib(vao, 2);
    gl::VertexArrayAttribFormat(vao, 2, 2, gl::FLOAT, gl::FALSE_, XR_U32_OFFSETOF(vertex_pnt, texcoord));
    gl::VertexArrayAttribBinding(vao, 2, 0);
}

void
xray::rendering::basic_mesh::update_vertices() noexcept
{
    assert(_mtype == mesh_type::writable);

    compute_bounding();

    scoped_resource_mapping vbmap{ vertex_buffer(), gl::MAP_WRITE_BIT, (uint32_t)container_bytes_size(_vertices) };

    if (!vbmap) {
        return;
    }

    memcpy(vbmap.memory(), _vertices.data(), container_bytes_size(_vertices));
}

void
xray::rendering::basic_mesh::update_indices() noexcept
{
    assert(_mtype == mesh_type::writable);

    scoped_resource_mapping ibmap{ index_buffer(), gl::MAP_WRITE_BIT, (uint32_t)container_bytes_size(_indices) };

    if (!ibmap) {
        return;
    }

    memcpy(ibmap.memory(), _indices.data(), container_bytes_size(_indices));
}

void
xray::rendering::basic_mesh::set_instance_data(const instance_descriptor* instances,
                                               const size_t instance_count,
                                               const vertex_attribute_descriptor* vertex_attributes,
                                               const size_t vertex_attributes_count)
{
    assert(valid());
    itlib::small_vector<GLuint, 16> buffers;
    itlib::small_vector<GLsizei, 16> strides;
    itlib::small_vector<GLintptr, 16> offsets;

    for (size_t idx = 0; idx < instance_count; ++idx) {
        const instance_descriptor& id = instances[idx];
        buffers.emplace_back(id.buffer_handle);
        strides.emplace_back(id.element_stride);
        offsets.emplace_back(id.buffer_offset);
    }

    const auto vao = raw_handle(_vertexarray);

    gl::VertexArrayVertexBuffers(
        vao, 1, static_cast<GLsizei>(instance_count), buffers.data(), offsets.data(), strides.data());

    unordered_map<GLuint, GLuint> buffer_bindpoints;

    for (size_t i = 0; i < instance_count; ++i) {
        gl::VertexArrayBindingDivisor(vao, i + 1, instances[i].instance_divisor);
        buffer_bindpoints[instances[i].buffer_handle] = static_cast<uint32_t>(i + 1);
    }

    std::span<const vertex_attribute_descriptor> attributes{
        vertex_attributes, vertex_attributes + static_cast<size_t>(vertex_attributes_count)
    };

    {
        const auto msg = "Setting vertex attributes for instance buffers";
        gl::DebugMessageInsert(
            gl::DEBUG_SOURCE_APPLICATION, gl::DEBUG_TYPE_MARKER, 0, gl::DEBUG_SEVERITY_NOTIFICATION, strlen(msg), msg);
    }

    for_each(begin(attributes), end(attributes), [vao, &buffer_bindpoints](const vertex_attribute_descriptor& attr) {
        //
        // 3 attributes already there by default : position, normal,
        // texture coords.
        const auto component_index = attr.component_index + 3;
        gl::VertexArrayAttribFormat(vao,
                                    component_index,
                                    attr.component_count,
                                    attr.component_type,
                                    attr.normalized ? gl::TRUE_ : gl::FALSE_,
                                    attr.component_offset);

        assert(buffer_bindpoints.find(attr.instance_desc->buffer_handle) != end(buffer_bindpoints));

        gl::VertexArrayAttribBinding(vao, component_index, buffer_bindpoints[attr.instance_desc->buffer_handle]);
        gl::EnableVertexArrayAttrib(vao, component_index);
    });
}
