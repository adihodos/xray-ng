#include "xray/rendering/mesh.hpp"
#include "xray/base/basic_timer.hpp"
#include "xray/base/dbg/debug_ext.hpp"
#include "xray/base/logger.hpp"
#include "xray/math/math_std.hpp"
#include "xray/math/objects/aabb3_math.hpp"
#include "xray/math/objects/sphere_math.hpp"
#include "xray/math/scalar2.hpp"
#include "xray/math/scalar3.hpp"
#include "xray/math/scalar3_math.hpp"
#include "xray/rendering/vertex_format/vertex_pnt.hpp"
#include <cassert>
#include <cmath>
#include <cstring>
#include <platformstl/filesystem/memory_mapped_file.hpp>
#include <random>
#include <tbb/tbb.h>
#include <unordered_map>
#include <vector>

#define TINYOBJ_LOADER_OPT_IMPLEMENTATION
#include "tinyobj/tinyobj_loader_opt.h"

using namespace std;
using namespace xray::base;
using namespace xray::math;
using namespace xray::rendering;
using namespace tinyobj_opt;

struct geometry_blob {
  std::vector<xray::rendering::vertex_pnt> vertices;
  std::vector<uint32_t>                    indices;
};

xray::rendering::basic_mesh::basic_mesh(
  const xray::rendering::vertex_pnt* vertices,
  const size_t                       num_vertices,
  const uint32_t*                    indices,
  const size_t                       num_indices) {

  _vertices.resize(num_vertices);
  _indices.resize(num_indices);

  memcpy(_vertices.data(), vertices, num_vertices * sizeof(vertices[0]));
  memcpy(_indices.data(), indices, num_indices * sizeof(indices[0]));

  compute_aabb();
  setup_buffers();
}

template <class T>
inline void hash_combine(std::size_t& s, const T& v) {
  std::hash<T> h;
  s ^= h(v) + 0x9e3779b9 + (s << 6) + (s >> 2);
}

namespace tinyobj_opt {

inline bool operator==(const tinyobj_opt::index_t& a,
                       const tinyobj_opt::index_t& b) {
  return a.vertex_index == b.vertex_index && a.normal_index == b.normal_index &&
         a.texcoord_index == b.texcoord_index;
}

inline bool operator!=(const tinyobj_opt::index_t& a,
                       const tinyobj_opt::index_t& b) {
  return !(a == b);
}

} // namespace tinyobj_opt

namespace std {
template <>
struct hash<tinyobj_opt::index_t> {
  using argument_type = tinyobj_opt::index_t;
  using result_type   = std::size_t;

  result_type operator()(const argument_type& a) const {
    result_type r{};
    hash_combine(r, a.vertex_index);
    hash_combine(r, a.normal_index);
    hash_combine(r, a.texcoord_index);

    return r;
  }
};
} // namespace std

xray::rendering::basic_mesh::basic_mesh(const char* path) {
  using namespace xray::rendering;

  attrib_t           attrs;
  vector<shape_t>    shapes;
  vector<material_t> mtls;

  platformstl::memory_mapped_file mmfile{path};

  const auto res = parseObj(
    &attrs, &shapes, &mtls, (const char*) mmfile.memory(), mmfile.size(), {});

  if (!res) {
    return;
  }

  _vertices.reserve(attrs.vertices.size());
  _indices.reserve(attrs.indices.size());

  unordered_map<index_t, uint32_t> idxmap;

  for_each(begin(attrs.indices), end(attrs.indices), [
    has_normals   = !attrs.normals.empty(),
    has_texcoords = !attrs.texcoords.empty(),
    a             = &attrs,
    &idxmap,
    this
  ](const index_t& idx) {

    auto it = idxmap.find(idx);
    if (it != end(idxmap)) {
      _indices.push_back(it->second);
    } else {
      vertex_pnt v;

      v.position = {a->vertices[idx.vertex_index * 3 + 0],
                    a->vertices[idx.vertex_index * 3 + 1],
                    a->vertices[idx.vertex_index * 3 + 2]};

      if (has_texcoords) {
        v.texcoord = {a->texcoords[idx.texcoord_index * 2 + 0],
                      1.0f - a->texcoords[idx.texcoord_index * 2 + 1]};
      } else {
        v.texcoord = vec2f::stdc::zero;
      }

      if (has_normals) {
        v.normal = {a->normals[idx.normal_index * 3 + 0],
                    a->normals[idx.normal_index * 3 + 1],
                    a->normals[idx.normal_index * 3 + 2]};
      } else {
        v.normal = vec3f::stdc::zero;
      }

      _vertices.push_back(v);
      idxmap[idx] = (uint32_t) _vertices.size() - 1;
      _indices.push_back((uint32_t) _vertices.size() - 1);
    }
  });

  if (attrs.normals.empty()) {
    assert((_indices.size() % 3) == 0);

    for (size_t i = 0, cnt = _indices.size() / 3; i < cnt; ++i) {
      auto& v0 = _vertices[_indices[i * 3 + 0]];
      auto& v1 = _vertices[_indices[i * 3 + 1]];
      auto& v2 = _vertices[_indices[i * 3 + 2]];

      const auto n =
        cross(v1.position - v0.position, v2.position - v0.position);

      v0.normal += n;
      v1.normal += n;
      v2.normal += n;
    }

    for_each(begin(_vertices), end(_vertices), [](vertex_pnt& v) {
      v.normal = normalize(v.normal);
    });
  }

  compute_aabb();
  setup_buffers();
}

void xray::rendering::basic_mesh::compute_aabb() {
  assert(!_vertices.empty());

  timer_highp op_tm;

  struct aabb_reducer {
  public:
    aabb_reducer() noexcept = default;

    aabb_reducer(const aabb_reducer& rhs, tbb::split) : _aabb{rhs._aabb} {}

    void operator()(const tbb::blocked_range<const vertex_pnt*>& rng) {
      for (auto b = rng.begin(), e = rng.end(); b < e; ++b) {
        _aabb.max = max(_aabb.max, b->position);
        _aabb.min = min(_aabb.min, b->position);
      }
    }

    void join(const aabb_reducer& other) {
      _aabb = math::merge(_aabb, other.bounding_box());
    }

    const aabb3f& bounding_box() const noexcept { return _aabb; }

  private:
    aabb3f _aabb{aabb3f::stdc::identity};
  };

  static constexpr size_t PARALLEL_REDUCE_MIN_VERTEX_COUNT = 120000u;

  if (_vertices.size() >= PARALLEL_REDUCE_MIN_VERTEX_COUNT) {
    scoped_timing_object<timer_highp> sto{&op_tm};

    aabb_reducer reducer{};
    tbb::parallel_reduce(
      tbb::blocked_range<const vertex_pnt*>{&_vertices[0],
                                            &_vertices[0] + _vertices.size()},
      reducer);

    _aabb = reducer.bounding_box();
  } else {
    scoped_timing_object<timer_highp> sto{&op_tm};
    _aabb =
      math::bounding_box3_axis_aligned((const math::vec3f*) _vertices.data(),
                                       _vertices.size(),
                                       sizeof(_vertices[0]));
  }

  OUTPUT_DBG_MSG("AABB : [%3.3f, %3.3f, %3.3f] : [%3.3f, "
                 "%3.3f, %3.3f]\n Time %3.3f",
                 _aabb.min.x,
                 _aabb.min.y,
                 _aabb.min.z,
                 _aabb.max.x,
                 _aabb.max.y,
                 _aabb.max.z,
                 op_tm.elapsed_millis());
}

void xray::rendering::basic_mesh::setup_buffers() {
  gl::CreateBuffers(1, raw_handle_ptr(_vertexbuffer));
  gl::NamedBufferStorage(raw_handle(_vertexbuffer),
                         xray::base::container_bytes_size(_vertices),
                         _vertices.data(),
                         0);

  gl::CreateBuffers(1, raw_handle_ptr(_indexbuffer));
  gl::NamedBufferStorage(raw_handle(_indexbuffer),
                         xray::base::container_bytes_size(_indices),
                         _indices.data(),
                         0);

  gl::CreateVertexArrays(1, raw_handle_ptr(_vertexarray));
  const auto vao = raw_handle(_vertexarray);

  gl::VertexArrayVertexBuffer(
    vao, 0, raw_handle(_vertexbuffer), 0, sizeof(vertex_pnt));
  gl::VertexArrayElementBuffer(vao, raw_handle(_indexbuffer));

  gl::EnableVertexArrayAttrib(vao, 0);
  gl::VertexArrayAttribFormat(
    vao, 0, 3, gl::FLOAT, gl::FALSE_, XR_U32_OFFSETOF(vertex_pnt, position));
  gl::VertexArrayAttribBinding(vao, 0, 0);

  gl::EnableVertexArrayAttrib(vao, 1);
  gl::VertexArrayAttribFormat(
    vao, 1, 3, gl::FLOAT, gl::FALSE_, XR_U32_OFFSETOF(vertex_pnt, normal));
  gl::VertexArrayAttribBinding(vao, 1, 0);

  gl::EnableVertexArrayAttrib(vao, 2);
  gl::VertexArrayAttribFormat(
    vao, 2, 2, gl::FLOAT, gl::FALSE_, XR_U32_OFFSETOF(vertex_pnt, texcoord));
  gl::VertexArrayAttribBinding(vao, 2, 0);
}