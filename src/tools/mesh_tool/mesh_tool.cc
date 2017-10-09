#include "xray/xray.hpp"
#include "xray/base/app_config.hpp"
#include "xray/base/dbg/debug_ext.hpp"
#include "xray/math/scalar2.hpp"
#include "xray/math/scalar3.hpp"
#include "xray/rendering/mesh.hpp"
#include "xray/rendering/vertex_format/vertex_pnt.hpp"
#include <algorithm>
#include <cstdint>
#include <functional>
#include <memory>
#include <platformstl/filesystem/path.hpp>
#include <platformstl/filesystem/readdir_sequence.hpp>
#include <unordered_map>
#include <utility>
#include <vector>

#define TINYOBJ_LOADER_OPT_IMPLEMENTATION
#include "tinyobj/tinyobj_loader_opt.h"

using namespace xray;
using namespace xray::base;
using namespace xray::rendering;
using namespace xray::math;
using namespace std;
using namespace platformstl;
using namespace tinyobj_opt;

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

static bool
load_mesh_obj_format(const char*                               file_path,
                     std::vector<xray::rendering::vertex_pnt>* vertices,
                     std::vector<uint32_t>*                    indices,
                     const uint32_t                            load_options =
                       xray::rendering::mesh_load_option::remove_points_lines) {

  using namespace xray::rendering;

  attrib_t           attrs;
  vector<shape_t>    shapes;
  vector<material_t> mtls;

  platformstl::memory_mapped_file mmfile{file_path};

  const auto res = parseObj(
    &attrs, &shapes, &mtls, (const char*) mmfile.memory(), mmfile.size(), {});

  if (!res) {
    return false;
  }

  vertices->reserve(attrs.vertices.size());
  indices->reserve(attrs.indices.size());

  unordered_map<index_t, uint32_t> idxmap;

  for_each(begin(attrs.indices), end(attrs.indices), [
    has_normals   = !attrs.normals.empty(),
    has_texcoords = !attrs.texcoords.empty(),
    a             = &attrs,
    &idxmap,
    vertices,
    indices
  ](const index_t& idx) {

    auto it = idxmap.find(idx);
    if (it != end(idxmap)) {
      indices->push_back(it->second);
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

      vertices->push_back(v);
      idxmap[idx] = (uint32_t) vertices->size() - 1;
      indices->push_back((uint32_t) vertices->size() - 1);
    }
  });

  if (attrs.normals.empty()) {
    assert((indices->size() % 3) == 0);

    for (size_t i = 0, cnt = indices->size() / 3; i < cnt; ++i) {
      auto& v0 = (*vertices)[(*indices)[i * 3 + 0]];
      auto& v1 = (*vertices)[(*indices)[i * 3 + 1]];
      auto& v2 = (*vertices)[(*indices)[i * 3 + 2]];

      const auto n =
        cross(v1.position - v0.position, v2.position - v0.position);

      v0.normal += n;
      v1.normal += n;
      v2.normal += n;
    }

    for_each(begin(*vertices), end(*vertices), [](vertex_pnt& v) {
      v.normal = normalize(v.normal);
    });
  }

  return true;
}

using unique_file = std::unique_ptr<FILE, decltype(&fclose)>;

int main(int argc, char** argv) {
  if (argc != 2) {
    return EXIT_FAILURE;
  }

  vector<vertex_pnt> vertices;
  vector<uint32_t>   indices;

  readdir_sequence dir_contents_iter{
    argv[1], readdir_sequence::files | readdir_sequence::fullPath};

  for_each(
    begin(dir_contents_iter),
    end(dir_contents_iter),
    [&vertices, &indices](const char* file) {
      basic_path<char> path{file};

      if (strcmp(path.get_ext(), "obj")) {
        return;
      }

      OUTPUT_DBG_MSG("Processing file %s", file);

      vertices.clear();
      indices.clear();

      if (!load_mesh_obj_format(
            file, &vertices, &indices, mesh_load_option::remove_points_lines)) {
        OUTPUT_DBG_MSG("Failed to load mesh!");
        return;
      }

      path.pop_ext();
      path.push_ext("bin");

      OUTPUT_DBG_MSG("Processing to file %s", path.c_str());

      unique_file outfile{fopen(path.c_str(), "wb"), &fclose};
      if (!outfile) {
        OUTPUT_DBG_MSG("Failed to open output file!");
        return;
      }

      struct mesh_header mhdr;
      mhdr.ver_major     = 1;
      mhdr.ver_minor     = 0;
      mhdr.index_count   = (uint32_t) indices.size();
      mhdr.vertex_count  = (uint32_t) vertices.size();
      mhdr.index_size    = sizeof(uint32_t);
      mhdr.vertex_size   = sizeof(vertex_pnt);
      mhdr.vertex_offset = sizeof(mhdr);
      mhdr.index_offset =
        mhdr.vertex_offset + mhdr.vertex_count * mhdr.vertex_size;

      fwrite(&mhdr, sizeof(mhdr), 1, outfile.get());
      fwrite(vertices.data(), vertices.size(), mhdr.vertex_size, outfile.get());
      fwrite(indices.data(), indices.size(), mhdr.index_size, outfile.get());

    });

  return 0;
}
