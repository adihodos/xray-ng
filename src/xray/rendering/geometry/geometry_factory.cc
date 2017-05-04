#include "xray/rendering/geometry/geometry_factory.hpp"
#include "xray/base/array_dimension.hpp"
#include "xray/base/dbg/debug_ext.hpp"
#include "xray/base/logger.hpp"
#include "xray/base/unique_pointer.hpp"
#include "xray/math/constants.hpp"
#include "xray/math/math_std.hpp"
#include "xray/math/scalar2.hpp"
#include "xray/math/scalar3.hpp"
#include "xray/math/scalar3_math.hpp"
#include "xray/rendering/geometry/geometry_data.hpp"
#include <cassert>
#include <cmath>
#include <cstring>
#include <platformstl/filesystem/memory_mapped_file.hpp>
#include <span.h>
#include <vector>

using namespace std;
using namespace xray::base;
using namespace xray::math;
using namespace xray::rendering;

static void
compute_normals(xray::rendering::geometry_data_t* geometry) noexcept {
  using namespace xray::math;

  assert(geometry != nullptr);
  assert((geometry->index_count % 3) == 0);

  for (size_t idx = 0; idx < geometry->vertex_count; ++idx) {
    geometry->geometry[idx].normal = vec3f::stdc::zero;
  }

  for (size_t tri_idx = 0; tri_idx < geometry->index_count / 3; ++tri_idx) {
    const vec3f& p0 =
      geometry->geometry[geometry->indices[tri_idx * 3 + 0]].position;
    const vec3f& p1 =
      geometry->geometry[geometry->indices[tri_idx * 3 + 1]].position;
    const vec3f& p2 =
      geometry->geometry[geometry->indices[tri_idx * 3 + 2]].position;

    const vec3f normal{cross(p1 - p0, p2 - p0)};

    geometry->geometry[geometry->indices[tri_idx * 3 + 0]].normal += normal;
    geometry->geometry[geometry->indices[tri_idx * 3 + 1]].normal += normal;
    geometry->geometry[geometry->indices[tri_idx * 3 + 2]].normal += normal;
  }

  for (size_t idx = 0; idx < geometry->vertex_count; ++idx) {
    geometry->geometry[idx].normal = normalize(geometry->geometry[idx].normal);
  }
}

static void subdivide_geometry(std::vector<vertex_pntt>* input_vertices,
                               std::vector<uint32_t>*    input_indices) {
  //
  //  Save a copy of the input geometry.
  vector<vertex_pntt> old_vertices{*input_vertices};
  vector<uint32_t>    old_indices{*input_indices};

  input_vertices->resize(0);
  input_indices->resize(0);

  /*
      v1
       .
      / \
     /   \
  m0.-----.m1
   / \   / \
  /   \ /   \
  .-----.-----.
  v0    m2     v2

   */

  const uint32_t triangle_count = static_cast<uint32_t>(old_indices.size() / 3);

  for (uint32_t i = 0; i < triangle_count; ++i) {
    const auto& v0 = old_vertices[old_indices[i * 3 + 0]];
    const auto& v1 = old_vertices[old_indices[i * 3 + 1]];
    const auto& v2 = old_vertices[old_indices[i * 3 + 2]];

    //
    // Generate the midpoints.
    vertex_pntt m0, m1, m2;

    // For subdivision, we just care about the position component.  We derive
    // the other vertex_pntt components in CreateGeosphere.

    m0.position = vec3f{0.5f * (v0.position.x + v1.position.x),
                        0.5f * (v0.position.y + v1.position.y),
                        0.5f * (v0.position.z + v1.position.z)};

    m1.position = vec3f{0.5f * (v1.position.x + v2.position.x),
                        0.5f * (v1.position.y + v2.position.y),
                        0.5f * (v1.position.z + v2.position.z)};

    m2.position = vec3f{0.5f * (v0.position.x + v2.position.x),
                        0.5f * (v0.position.y + v2.position.y),
                        0.5f * (v0.position.z + v2.position.z)};

    //
    // Add new geometry.

    input_vertices->push_back(v0); // 0
    input_vertices->push_back(v1); // 1
    input_vertices->push_back(v2); // 2
    input_vertices->push_back(m0); // 3
    input_vertices->push_back(m1); // 4
    input_vertices->push_back(m2); // 5

    input_indices->push_back(i * 6 + 0);
    input_indices->push_back(i * 6 + 5);
    input_indices->push_back(i * 6 + 3);

    input_indices->push_back(i * 6 + 3);
    input_indices->push_back(i * 6 + 5);
    input_indices->push_back(i * 6 + 4);

    input_indices->push_back(i * 6 + 5);
    input_indices->push_back(i * 6 + 2);
    input_indices->push_back(i * 6 + 4);

    input_indices->push_back(i * 6 + 3);
    input_indices->push_back(i * 6 + 4);
    input_indices->push_back(i * 6 + 1);
  }
}

// void xray::rendering::geometry_factory::create_circle(
//     const float radius, const uint32_t tess_factor, geometry_data_t* mesh)
//     {
//   const uint32_t circle_slices = tess_factor < 3U ? 3U : tess_factor;

//   mesh->setup(circle_slices + 2, circle_slices * 3);

//   const float theta_step = two_pi<float>() / circle_slices;

//   mesh->geometry[0].position  = math::vec3f::null;
//   mesh->geometry[0].normal    = math::vec3f::unit_y;
//   mesh->geometry[0].tangent   = math::vec3f::unit_x;
//   mesh->geometry[0].texcoords = math::vec2f{0.5f, 0.5f};

//   //
//   //  need to duplicate last vertex, otherwise texture coordinates and
//   //  tangents won't be correct.
//   for (size_t idx = 1; idx < circle_slices + 2; ++idx) {
//     mesh->geometry[idx].position =
//         math::vec3f{radius * cos((idx - 1) * theta_step), 0.0f,
//                      radius * sin((idx - 1) * theta_step)};

//     mesh->geometry[idx].normal = math::vec3f::unit_y;

//     mesh->geometry[idx].tangent =
//         math::vec3f{-radius * sin((idx - 1) * theta_step), 0.0f,
//                      radius * cos((idx - 1) * theta_step)};

//     mesh->geometry[idx].texcoords =
//         math::vec2f{cos((idx - 1) * theta_step) * 0.5f + 0.5f,
//                      sin((idx - 1) * theta_step) * 0.5f + 0.5f};
//   }

//   for (uint32_t idx = 0; idx < circle_slices; ++idx) {
//     mesh->indices[idx * 3 + 0] = 0;
//     mesh->indices[idx * 3 + 1] = idx + 1;
//     mesh->indices[idx * 3 + 2] = idx + 2;
//   }
// }

void xray::rendering::geometry_factory::cylinder(
  const uint32_t   ring_tesselation_factor,
  const uint32_t   vertical_rings,
  const float      height,
  const float      radius,
  geometry_data_t* cylinder) {

  assert(ring_tesselation_factor >= 3);
  assert(cylinder != nullptr);

  const auto ring_vertexcount = ring_tesselation_factor + 1u;
  const auto ring_count       = vertical_rings + 2u;

  const auto vertex_count = ring_vertexcount * ring_count;
  const auto face_count   = ring_tesselation_factor * (ring_count - 1u);
  const auto index_count  = face_count * 6u;

  const float delta_y = height / static_cast<float>(ring_count - 1u);
  const float delta_xz =
    two_pi<float> / static_cast<float>(ring_tesselation_factor);

  cylinder->setup(vertex_count, index_count);

  auto vertex_ptr = raw_ptr(cylinder->geometry);

  for (uint32_t i = 0; i < ring_count; ++i) {
    const float vtx_y = static_cast<float>(i) * delta_y - 0.5f * height;

    for (uint32_t j = 1; j <= ring_vertexcount; ++j) {
      const float theta = static_cast<float>(j - 1) * delta_xz;

      const float sin_theta = std::sin(theta);
      const float cos_theta = std::cos(theta);

      const float vtx_x = radius * cos_theta;
      const float vtx_z = radius * sin_theta;

      vertex_ptr->position = vec3f{vtx_x, vtx_y, vtx_z};

      vertex_ptr->tangent = vec3f{-sin_theta, 0.0f, cos_theta};
      //      const auto bitangent = vec3f{0.0f, height, 0.0f};

      vertex_ptr->normal = normalize(vertex_ptr->position);
      // vec3f{cos_theta, 0.0f, sin_theta};
      // normalize(cross_product(bitangent, tangent));

      vertex_ptr->texcoords.x = theta / two_pi<float>;
      vertex_ptr->texcoords.y =
        1.0f - static_cast<float>(i) / static_cast<float>(ring_count - 1);

      ++vertex_ptr;
    }
  }

  auto index_ptr = raw_ptr(cylinder->indices);
  auto last_idx  = index_ptr + index_count;

  auto write_index_fn = [ring_vertexcount,
                         last_idx](uint32_t i, uint32_t j, uint32_t* idx_ptr) {
    assert((idx_ptr + 6) <= last_idx);

    *idx_ptr++ = i * ring_vertexcount + j;
    *idx_ptr++ = (i + 1) * ring_vertexcount + j + 1;
    *idx_ptr++ = (i + 1) * ring_vertexcount + j;

    *idx_ptr++ = i * ring_vertexcount + j;
    *idx_ptr++ = i * ring_vertexcount + j + 1;
    *idx_ptr++ = (i + 1) * ring_vertexcount + j + 1;

    return idx_ptr;
  };

  for (uint32_t i = 0; i < (ring_count - 1u); ++i) {
    for (uint32_t j = 0; j < ring_tesselation_factor; ++j) {
      index_ptr = write_index_fn(i, j, index_ptr);
    }
  }
}

void xray::rendering::geometry_factory::box(const float      width,
                                            const float      height,
                                            const float      depth,
                                            geometry_data_t* mesh_data) {
  mesh_data->setup(24, 36);

  const float w2 = 0.5f * width;
  const float h2 = 0.5f * height;
  const float d2 = 0.5f * depth;

  auto v = base::raw_ptr(mesh_data->geometry);
  //      gsl::span<vertex_pntt>{base::raw_ptr(mesh_data->geometry),
  //                             static_cast<ptrdiff_t>(mesh_data->vertex_count)};

  // Fill in the front face vertex data.
  v[0] =
    vertex_pntt(-w2, -h2, -d2, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
  v[1] =
    vertex_pntt(-w2, +h2, -d2, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
  v[2] =
    vertex_pntt(+w2, +h2, -d2, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f);
  v[3] =
    vertex_pntt(+w2, -h2, -d2, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f);

  // Fill in the back face vertex data.
  v[4] =
    vertex_pntt(-w2, -h2, +d2, 0.0f, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f);
  v[5] =
    vertex_pntt(+w2, -h2, +d2, 0.0f, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
  v[6] =
    vertex_pntt(+w2, +h2, +d2, 0.0f, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
  v[7] =
    vertex_pntt(-w2, +h2, +d2, 0.0f, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f);

  // Fill in the top face vertex data.
  v[8] =
    vertex_pntt(-w2, +h2, -d2, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
  v[9] =
    vertex_pntt(-w2, +h2, +d2, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
  v[10] =
    vertex_pntt(+w2, +h2, +d2, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f);
  v[11] =
    vertex_pntt(+w2, +h2, -d2, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f);

  // Fill in the bottom face vertex data.
  v[12] = vertex_pntt(
    -w2, -h2, -d2, 0.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f);
  v[13] = vertex_pntt(
    +w2, -h2, -d2, 0.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
  v[14] = vertex_pntt(
    +w2, -h2, +d2, 0.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
  v[15] = vertex_pntt(
    -w2, -h2, +d2, 0.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f);

  // Fill in the left face vertex data.
  v[16] = vertex_pntt(
    -w2, -h2, +d2, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f);
  v[17] = vertex_pntt(
    -w2, +h2, +d2, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f);
  v[18] = vertex_pntt(
    -w2, +h2, -d2, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f);
  v[19] = vertex_pntt(
    -w2, -h2, -d2, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f);

  // Fill in the right face vertex data.
  v[20] =
    vertex_pntt(+w2, -h2, -d2, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f);
  v[21] =
    vertex_pntt(+w2, +h2, -d2, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f);
  v[22] =
    vertex_pntt(+w2, +h2, +d2, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f);
  v[23] =
    vertex_pntt(+w2, -h2, +d2, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);

  //
  // Create the indices.
  auto i = raw_ptr(mesh_data->indices);
  //          gsl::span<uint32_t>{raw_ptr(mesh_data->indices),
  //                               static_cast<ptrdiff_t>(mesh_data->index_count)};

  // Fill in the front face index data
  i[0] = 0;
  i[1] = 1;
  i[2] = 2;
  i[3] = 0;
  i[4] = 2;
  i[5] = 3;

  // Fill in the back face index data
  i[6]  = 4;
  i[7]  = 5;
  i[8]  = 6;
  i[9]  = 4;
  i[10] = 6;
  i[11] = 7;

  // Fill in the top face index data
  i[12] = 8;
  i[13] = 9;
  i[14] = 10;
  i[15] = 8;
  i[16] = 10;
  i[17] = 11;

  // Fill in the bottom face index data
  i[18] = 12;
  i[19] = 13;
  i[20] = 14;
  i[21] = 12;
  i[22] = 14;
  i[23] = 15;

  // Fill in the left face index data
  i[24] = 16;
  i[25] = 17;
  i[26] = 18;
  i[27] = 16;
  i[28] = 18;
  i[29] = 19;

  // Fill in the right face index data
  i[30] = 20;
  i[31] = 21;
  i[32] = 22;
  i[33] = 20;
  i[34] = 22;
  i[35] = 23;
}

// void xray::rendering::geometry_factory::create_conical_shape(
//     const float height, const float radius, const uint32_t slices,
//     const uint32_t stacks, geometry_data_t* mesh) {
//   const uint32_t vertex_count = (slices + 1) * (stacks + 1) + 1;
//   const uint32_t index_count =
//       slices * 3 + (slices - 2) * 3 + stacks * slices * 3 * 2;

//   mesh->setup(vertex_count, index_count);

//   mesh->geometry[0].position  = math::vec3f{0.0f, height, 0.0f};
//   mesh->geometry[0].normal    = math::vec3f::unit_y;
//   mesh->geometry[0].tangent   = math::vec3f::unit_x;
//   mesh->geometry[0].texcoords = math::vec2f{0.5f, 0.5f};

//   const float theta_step  = math::numericsF::two_pi() / slices;
//   const float height_step = height / (stacks + 1);
//   const float radius_step = radius / (stacks + 1);

//   //
//   //  need to duplicate last vertex, otherwise texture coordinates and
//   //  tangents won't be correct.
//   vertex_pntt_t* vertex_ptr = base::raw_ptr(mesh->geometry) + 1;
//   for (uint32_t idx = 1; idx <= slices + 1; ++idx, ++vertex_ptr) {
//     vertex_ptr->position = math::vec3f{
//         radius_step * cos((idx - 1) * theta_step), height - height_step,
//         radius_step * sin((idx - 1) * theta_step)};

//     vertex_ptr->normal = math::vec3f::unit_y;

//     vertex_ptr->tangent =
//         math::vec3f{-radius_step * sin((idx - 1) * theta_step), 0.0f,
//                      radius_step * cos((idx - 1) * theta_step)};

//     vertex_ptr->texcoords =
//         math::vec2f{cos((idx - 1) * theta_step) * 0.5f + 0.5f,
//                      sin((idx - 1) * theta_step) * 0.5f + 0.5f};
//   }

//   for (uint32_t idx = 1; idx <= stacks; ++idx) {
//     const float stk_height = height - height_step * (idx + 1);
//     const float stk_radius = (idx + 1) * radius_step;

//     for (uint32_t j = 1; j <= slices + 1; ++j, ++vertex_ptr) {
//       vertex_ptr->position =
//           math::vec3f{stk_radius * cos((j - 1) * theta_step), stk_height,
//                        stk_radius * sin((j - 1) * theta_step)};

//       vertex_ptr->tangent =
//           math::vec3f{-stk_radius * sin((j - 1) * theta_step), 0.0f,
//                        stk_radius * cos((j - 1) * theta_step)};

//       vertex_ptr->texcoords =
//           math::vec2f{cos((j - 1) * theta_step) * 0.5f + 0.5f,
//                        sin((j - 1) * theta_step) * 0.5f + 0.5f};
//     }
//   }

//   //
//   //  Top part.
//   uint32_t* index_ptr = base::raw_ptr(mesh->indices);
//   for (uint32_t idx = 0; idx < slices; ++idx) {
//     *index_ptr++ = 0;
//     *index_ptr++ = idx + 1;
//     *index_ptr++ = idx + 2;
//   }

//   //
//   //  Stacks
//   const uint32_t offset_stk       = 1;
//   const uint32_t ring_vertexcount = slices + 1;
//   for (uint32_t i = 0; i < stacks; ++i) {
//     for (uint32_t j = 0; j < slices; ++j) {
//       *index_ptr++ = offset_stk + (i + 1) * ring_vertexcount + j;
//       *index_ptr++ = offset_stk + i * ring_vertexcount + j + 1;
//       *index_ptr++ = offset_stk + i * ring_vertexcount + j;

//       *index_ptr++ = offset_stk + (i + 1) * ring_vertexcount + j;
//       *index_ptr++ = offset_stk + (i + 1) * ring_vertexcount + j + 1;
//       *index_ptr++ = offset_stk + i * ring_vertexcount + j + 1;
//     }
//   }

//   //
//   //  bottom cap
//   const uint32_t offset = 1 + stacks * (slices + 1);
//   for (uint32_t idx = 0; idx < slices - 2; ++idx) {
//     *index_ptr++ = offset;
//     *index_ptr++ = offset + idx + 2;
//     *index_ptr++ = offset + idx + 1;
//   }
// }

void xray::rendering::geometry_factory::grid(const float      grid_width,
                                             const float      grid_depth,
                                             const size_t     row_count,
                                             const size_t     column_count,
                                             geometry_data_t* mesh_data) {

  const size_t vertices_per_row = row_count + 1;
  const size_t vertices_per_col = column_count + 1;

  const size_t vertex_count = vertices_per_row * vertices_per_col;
  const size_t face_count   = row_count * column_count * 2;

  const float half_width = 0.5f * grid_width;
  const float half_depth = 0.5f * grid_depth;

  const float delta_x = grid_width / column_count;
  const float delta_z = grid_depth / row_count;

  const float delta_tu = 1.0f / column_count;
  const float delta_tv = 1.0f / row_count;

  mesh_data->setup(static_cast<uint32_t>(vertex_count),
                   static_cast<uint32_t>(face_count * 3));

  // typedef tbb::blocked_range2d<size_t, size_t> grid_domain_t;

  // tbb::parallel_for(
  //   grid_domain_t{0, vertices_per_row, 0, vertices_per_col},
  //   [=](const grid_domain_t& thread_subdomain) {
  //     for (size_t row_idx = thread_subdomain.rows().begin();
  //          row_idx < thread_subdomain.rows().end();
  //          ++row_idx) {
  //       const float z_coord = half_depth - row_idx * delta_z;

  //       for (size_t col_idx = thread_subdomain.cols().begin();
  //            col_idx < thread_subdomain.cols().end();
  //            ++col_idx) {
  //         vertex_pntt& output_vtx =
  //           mesh_data->geometry[row_idx * vertices_per_col + col_idx];

  //         const float x_coord = -half_width + col_idx * delta_x;

  //         output_vtx.position = math::vec3f{x_coord, 0.0f, z_coord};
  //         output_vtx.normal   = math::vec3f::stdc::unit_y;
  //         output_vtx.tangent  = math::vec3f::stdc::unit_x;
  //         output_vtx.texcoords =
  //           math::vec2f{col_idx * delta_tu, row_idx * delta_tv};
  //       }
  //     }
  //   });

  for (size_t row_idx = 0; row_idx < vertices_per_row; ++row_idx) {
    const float z_coord = half_depth - row_idx * delta_z;

    for (size_t col_idx = 0; col_idx < vertices_per_col; ++col_idx) {
      vertex_pntt& output_vtx =
        mesh_data->geometry[row_idx * vertices_per_col + col_idx];

      const float x_coord = -half_width + col_idx * delta_x;

      output_vtx.position = math::vec3f{x_coord, 0.0f, z_coord};
      output_vtx.normal   = math::vec3f::stdc::unit_y;
      output_vtx.tangent  = math::vec3f::stdc::unit_x;
      output_vtx.texcoords =
        math::vec2f{col_idx * delta_tu, row_idx * delta_tv};
    }
  }

  //
  // Setup indices for the generated vertices.
  auto index_array =
    gsl::span<uint32_t>(raw_ptr(mesh_data->indices), mesh_data->index_count);

  size_t quad_idx = 0;
  for (size_t row_idx = 0; row_idx < row_count; ++row_idx) {
    for (size_t col_idx = 0; col_idx < column_count; ++col_idx) {

      //
      //  First face.
      index_array[quad_idx] =
        static_cast<uint32_t>((row_idx + 1) * vertices_per_col + col_idx);

      index_array[quad_idx + 1] =
        static_cast<uint32_t>(row_idx * vertices_per_col + col_idx + 1);

      index_array[quad_idx + 2] =
        static_cast<uint32_t>(row_idx * vertices_per_col + col_idx);

      //
      //  Second face
      index_array[quad_idx + 3] =
        static_cast<uint32_t>((row_idx + 1) * vertices_per_col + col_idx);

      index_array[quad_idx + 4] =
        static_cast<uint32_t>((row_idx + 1) * vertices_per_col + col_idx + 1);

      index_array[quad_idx + 5] =
        static_cast<uint32_t>(row_idx * vertices_per_col + col_idx + 1);

      quad_idx += 6; // next quad
    }
  }
}

void xray::rendering::geometry_factory::fullscreen_quad(
  geometry_data_t* grid_geometry) {
  grid_geometry->setup(4, 6);

  constexpr float coords[] = {
    -1.0f, -1.0f, -1.0f, +1.0f, +1.0f, +1.0f, +1.0f, -1.0f};

  constexpr math::vec3f normal_vec{0.0f, 0.0f, -1.0f};
  constexpr float       texcoords[] = {
    0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f};

  for (size_t idx = 0; idx < 4; ++idx) {
    grid_geometry->geometry[idx].position =
      math::vec3f{coords[idx * 2 + 0], coords[idx * 2 + 1], 0.0f};

    grid_geometry->geometry[idx].normal  = normal_vec;
    grid_geometry->geometry[idx].tangent = vec3f::stdc::unit_x;
    grid_geometry->geometry[idx].texcoords =
      math::vec2f{texcoords[idx * 2 + 0], texcoords[idx * 2 + 1]};
  }

  static constexpr uint32_t fsquad_indices[] = {0, 2, 1, 0, 3, 2};

  memcpy(
    raw_ptr(grid_geometry->indices), fsquad_indices, sizeof(fsquad_indices));
}

// void xray::rendering::geometry_factory::create_spherical_shape(
//     const float radius, const uint32_t tess_factor_horz,
//     const uint32_t tess_factor_vert, geometry_data_t* mesh) {
//   assert(tess_factor_horz > 2);
//   assert(tess_factor_vert > 0);

//   const uint32_t ring_vertexcount = tess_factor_horz + 1;
//   const uint32_t vertex_count     = 2 + tess_factor_vert *
//   ring_vertexcount;
//   const uint32_t index_count      = tess_factor_horz * 3 * 2 *
//   tess_factor_vert;

//   mesh->setup(vertex_count, index_count);

//   vertex_pntt_t* vertices_ptr = base::raw_ptr(mesh->geometry);

//   //
//   //  North pole vertex.
//   *vertices_ptr++ = vertex_pntt_t{0.0f, +radius, 0.0f, 0.0f, +1.0f, 0.0f,
//                                   1.0f, 0.0f,    0.0f, 0.0f, 0.0f};

//   const float theta_step = math::numericsF::two_pi() / tess_factor_horz;
//   const float phi_step   = math::numericsF::pi() / (tess_factor_vert + 1);

//   for (uint32_t i = 1; i <= tess_factor_vert; ++i) {
//     const float phi = i * phi_step;

//     for (uint32_t j = 1; j <= tess_factor_horz + 1; ++j) {
//       const float theta = j * theta_step;

//       vertex_pntt_t sphere_vertex;

//       // spherical to cartesian
//       sphere_vertex.position.X = radius * sinf(phi) * cosf(theta);
//       sphere_vertex.position.Y = radius * cosf(phi);
//       sphere_vertex.position.Z = radius * sinf(phi) * sinf(theta);

//       // Partial derivative of P with respect to theta
//       sphere_vertex.tangent.X = -radius * sinf(phi) * sinf(theta);
//       sphere_vertex.tangent.Y = 0.0f;
//       sphere_vertex.tangent.Z = +radius * sinf(phi) * cosf(theta);
//       sphere_vertex.tangent.normalize();

//       sphere_vertex.normal = sphere_vertex.position;
//       sphere_vertex.normal.normalize();

//       sphere_vertex.texcoords.X = theta / math::numericsF::two_pi();
//       sphere_vertex.texcoords.Y = phi / math::numericsF::pi();

//       *vertices_ptr++ = sphere_vertex;
//     }
//   }

//   //
//   //  South pole vertex.
//   *vertices_ptr++ = vertex_pntt_t{0.0f, -radius, 0.0f, 0.0f, -1.0f, 0.0f,
//                                   1.0f, 0.0f,    0.0f, 0.0f, 1.0f};

//   uint32_t* index_itr = base::raw_ptr(mesh->indices);

//   //
//   //  Top cap
//   for (uint32_t i = 0; i < tess_factor_horz; ++i) {
//     *index_itr++ = 0;
//     *index_itr++ = i + 1;
//     *index_itr++ = i + 2;
//   }

//   //
//   //  Rings
//   const uint32_t offset_rings = 1;
//   for (uint32_t i = 0; i < tess_factor_vert - 1; ++i) {
//     for (uint32_t j = 0; j < tess_factor_horz; ++j) {
//       *index_itr++ = offset_rings + (i + 1) * ring_vertexcount + j;
//       *index_itr++ = offset_rings + i * ring_vertexcount + j + 1;
//       *index_itr++ = offset_rings + i * ring_vertexcount + j;

//       *index_itr++ = offset_rings + (i + 1) * ring_vertexcount + j;
//       *index_itr++ = offset_rings + (i + 1) * ring_vertexcount + j + 1;
//       *index_itr++ = offset_rings + i * ring_vertexcount + j + 1;
//     }
//   }

//   //
//   //  Bottom cap.
//   const uint32_t bottom_offset = 0 + (tess_factor_vert - 1) *
//   ring_vertexcount;
//   const uint32_t south_pole    = vertex_count - 1;
//   for (uint32_t i = 0; i < tess_factor_horz; ++i) {
//     *index_itr++ = south_pole;
//     *index_itr++ = bottom_offset + i + 2;
//     *index_itr++ = bottom_offset + i + 1;
//   }
// }

// void xray::rendering::geometry_factory::create_conical_section(
//     const float upper_radius, const float lower_radius, const float height,
//     const uint32_t tess_factor_horz, const uint32_t tess_factor_vert,
//     geometry_data_t* mesh) {
//   assert(upper_radius > 0.0f);
//   assert(lower_radius > 0.0f);
//   assert(height > 0.0f);

//   assert(tess_factor_horz > 2);

//   const uint32_t ring_vertexcount = tess_factor_horz + 1;
//   const uint32_t vertex_count     = (tess_factor_vert + 2) *
//   ring_vertexcount;
//   const uint32_t index_count =
//       (tess_factor_horz - 2) * 3 * 2 +
//       (tess_factor_vert + 1) * tess_factor_horz * 2 * 3;

//   mesh->setup(vertex_count, index_count);

//   const float    theta_step = math::numericsF::two_pi() / tess_factor_horz;
//   vertex_pntt_t* vertex_itr = base::raw_ptr(mesh->geometry);

//   for (uint32_t i = 0; i < tess_factor_vert + 2; ++i) {
//     const float tval = (float) (i) / (float) (tess_factor_vert + 1);
//     const float ring_radius =
//         math::linear_interpolate(upper_radius, lower_radius, tval);
//     const float ring_height = math::linear_interpolate(height, 0.0f, tval);

//     for (uint32_t j = 1; j <= tess_factor_horz + 1; ++j) {
//       const float theta = j * theta_step;

//       vertex_pntt_t vertex;

//       // spherical to cartesian
//       vertex.position.X = ring_radius * cosf(theta);
//       vertex.position.Y = ring_height;
//       vertex.position.Z = ring_radius * sinf(theta);

//       // Partial derivative of P with respect to theta
//       vertex.tangent.X = -ring_radius * sinf(theta);
//       vertex.tangent.Y = 0.0f;
//       vertex.tangent.Z = +ring_radius * cosf(theta);
//       vertex.tangent.normalize();

//       vertex.normal = vertex.position;
//       vertex.normal.normalize();

//       vertex.texcoords.X = theta / math::numericsF::two_pi();
//       vertex.texcoords.Y = ring_height / height;

//       *vertex_itr++ = vertex;
//     }
//   }

//   uint32_t* index_ptr = base::raw_ptr(mesh->indices);

//   //
//   //  Top cap
//   for (uint32_t i = 0; i < tess_factor_horz - 2; ++i) {
//     *index_ptr++ = 0;
//     *index_ptr++ = i + 1;
//     *index_ptr++ = i + 2;
//   }

//   //
//   //  Rings.
//   const uint32_t ring_offset = 0;
//   for (uint32_t i = 0; i < tess_factor_vert + 1; ++i) {
//     for (uint32_t j = 0; j < tess_factor_horz; ++j) {
//       *index_ptr++ = ring_offset + (i + 1) * ring_vertexcount + j;
//       *index_ptr++ = ring_offset + i * ring_vertexcount + j + 1;
//       *index_ptr++ = ring_offset + i * ring_vertexcount + j;

//       *index_ptr++ = ring_offset + (i + 1) * ring_vertexcount + j;
//       *index_ptr++ = ring_offset + (i + 1) * ring_vertexcount + j + 1;
//       *index_ptr++ = ring_offset + i * ring_vertexcount + j + 1;
//     }
//   }

//   //
//   //  Bottom cap
//   const uint32_t offset = vertex_count - tess_factor_horz - 1;
//   for (uint32_t j = 0; j < tess_factor_horz - 2; ++j) {
//     *index_ptr++ = offset;
//     *index_ptr++ = offset + j + 2;
//     *index_ptr++ = offset + j + 1;
//   }
// }

void xray::rendering::geometry_factory::geosphere(
  const float radius, const uint32_t max_subdivisions, geometry_data_t* mesh) {

  assert(mesh != nullptr);
  assert(radius > 0.0f);

  const uint32_t subdivisions =
    math::clamp(max_subdivisions, max_subdivisions, uint32_t{5});

  //
  // Approximate a sphere by tessellating an icosahedron.
  constexpr auto xpos = 0.525731f;
  constexpr auto zpos = 0.850651f;

  constexpr vec3f pos[12] = {math::vec3f{-xpos, 0.0f, +zpos},
                             math::vec3f{+xpos, 0.0f, +zpos},
                             math::vec3f{-xpos, 0.0f, -zpos},
                             math::vec3f{+xpos, 0.0f, -zpos},
                             math::vec3f{0.0f, +zpos, +xpos},
                             math::vec3f{0.0f, +zpos, -xpos},
                             math::vec3f{0.0f, -zpos, +xpos},
                             math::vec3f{0.0f, -zpos, -xpos},
                             math::vec3f{+zpos, +xpos, 0.0f},
                             math::vec3f{-zpos, +xpos, 0.0f},
                             math::vec3f{+zpos, -xpos, 0.0f},
                             math::vec3f{-zpos, -xpos, 0.0f}};

  constexpr uint32_t k[60] = {
    1,  0, 4, 4, 0, 9,  4, 9, 5,  8, 4, 5, 1,  4,  8, 1, 8, 10, 10, 8,
    3,  8, 5, 3, 3, 5,  2, 3, 2,  7, 3, 7, 10, 10, 7, 6, 6, 7,  11, 6,
    11, 0, 6, 0, 1, 10, 6, 1, 11, 9, 0, 2, 9,  11, 5, 9, 2, 11, 7,  2,
  };

  //
  //  TODO : Suboptimal code, rewrite when possible.
  vector<vertex_pntt> vertices{12};
  vector<uint32_t>    indices{begin(k), end(k)};

  for (uint32_t i = 0; i < 12; ++i) {
    vertices[i].position = pos[i];
  }

  for (uint32_t i = 0; i < subdivisions; ++i)
    subdivide_geometry(&vertices, &indices);

  //
  // Project md_vertices onto sphere and scale.
  for (uint32_t i = 0; i < static_cast<uint32_t>(vertices.size()); ++i) {
    vertices[i].normal   = normalize(vertices[i].position);
    vertices[i].position = radius * vertices[i].normal;

    //
    // Derive texture coordinates from spherical coordinates.
    const float theta =
      angle_from_xy(vertices[i].position.x, vertices[i].position.z);

    const float phi = acosf(vertices[i].position.x / radius);

    vertices[i].texcoords.x = theta / two_pi<float>;
    vertices[i].texcoords.y = phi / pi<float>;

    //
    // Partial derivative of P with respect to theta
    vertices[i].tangent.x = -radius * sinf(phi) * sinf(theta);
    vertices[i].tangent.y = 0.0f;
    vertices[i].tangent.z = +radius * sinf(phi) * cosf(theta);
    vertices[i].tangent   = normalize(vertices[i].tangent);
  }

  mesh->setup(vertices.size(), indices.size());
  std::copy(begin(vertices), end(vertices), base::raw_ptr(mesh->geometry));
  std::copy(begin(indices), end(indices), base::raw_ptr(mesh->indices));
}

void xray::rendering::geometry_factory::tetrahedron(geometry_data_t* mesh) {
  mesh->setup(4u, 12u);

  //  TODO : fix tangents and texture coordinates
  mesh->geometry[0].position = vec3f{0.0f, 0.0f, -1.0f};
  mesh->geometry[1].position = vec3f{0.9428f, 0.0f, 0.333f};
  mesh->geometry[2].position = vec3f{-0.4714045f, 0.81649658f, 0.333f};
  mesh->geometry[3].position = vec3f{-0.4714045f, -0.81649658f, 0.333f};

  constexpr uint32_t indices[] = {0, 1, 2, 0, 2, 3, 0, 3, 1, 1, 3, 2};

  memcpy(base::raw_ptr(mesh->indices), indices, sizeof(indices));

  compute_normals(mesh);
}

void xray::rendering::geometry_factory::hexahedron(geometry_data_t* mesh) {
  mesh->setup(8u, 36u);

  const vec3f vertices[] = {vec3f{-1.0f, -1.0f, -1.0f} * 0.57735f,
                            vec3f{+1.0f, -1.0f, -1.0f} * 0.57735f,
                            vec3f{+1.0f, +1.0f, -1.0f} * 0.57735f,
                            vec3f{-1.0f, +1.0f, -1.0f} * 0.57735f,
                            vec3f{-1.0f, -1.0f, +1.0f} * 0.57735f,
                            vec3f{+1.0f, -1.0f, +1.0f} * 0.57735f,
                            vec3f{+1.0f, +1.0f, +1.0f} * 0.57735f,
                            vec3f{-1.0f, +1.0f, +1.0f} * 0.57735f};

  for (size_t idx = 0; idx < XR_COUNTOF__(vertices); ++idx) {
    mesh->geometry[idx].position = vertices[idx];
  }

  constexpr uint32_t indices[] = {
    // clang-format off
      0, 3, 2,
      0, 2, 1,
      0, 1, 5,
      0, 5, 4,
      0, 4, 7,
      0, 7, 3,
      6, 5, 1,
      6, 1, 2,
      6, 2, 3,
      6, 3, 7,
      6, 7, 4,
      6, 4, 5
    // clang-format on
  };

  memcpy(base::raw_ptr(mesh->indices), indices, sizeof(indices));
  compute_normals(mesh);
}

void xray::rendering::geometry_factory::octahedron(geometry_data_t* mesh) {
  mesh->setup(6u, 24u);

  constexpr vec3f vertices[] = {vec3f{1.0f, 0.0f, 0.0f},
                                vec3f{-1.0f, 0.0f, 0.0f},
                                vec3f{0.0f, 1.0f, 0.0f},
                                vec3f{0.0f, -1.0f, 0.0f},
                                vec3f{0.0f, 0.0f, 1.0f},
                                vec3f{0.0f, 0.0f, -1.0f}};

  constexpr uint32_t indices[] = {
    // clang-format off
      4, 0, 2,
      4, 2, 1,
      4, 1, 3,
      4, 3, 0,
      5, 2, 0,
      5, 1, 2,
      5, 3, 1,
      5, 0, 3
    // clang-format on
  };

  for (size_t idx = 0; idx < XR_COUNTOF__(vertices); ++idx) {
    mesh->geometry[idx].position = vertices[idx];
  }

  memcpy(base::raw_ptr(mesh->indices), indices, sizeof(indices));
  compute_normals(mesh);
}

void xray::rendering::geometry_factory::dodecahedron(geometry_data_t* mesh) {
  mesh->setup(20u, 36u * 3u);

  constexpr auto a = 0.5773502691896258f;
  constexpr auto b = 0.3568220897730899f;
  constexpr auto c = 0.9341723589627158f;

  constexpr vec3f vertices[] = {
    // clang-format off
      vec3f{a, a, a},
      vec3f{a, a, -a},
      vec3f{a, -a, a},

      vec3f{a, -a, -a},
      vec3f{-a, a, a},
      vec3f{-a, a, -a},

      vec3f{-a, -a, a},
      vec3f{-a, -a, -a},
      vec3f{b, c, 0.0f},

      vec3f{-b, c, 0.0f},
      vec3f{b, -c, 0.0f},
      vec3f{-b, -c, 0.0f},

      vec3f{c, 0.0f, b},
      vec3f{c, 0.0f, -b},
      vec3f{-c, 0.0f, b},

      vec3f{-c, 0.0f, -b},
      vec3f{0.0f, b, c},
      vec3f{0.0f, -b, c},
      vec3f{0.0f, b, -c},
      vec3f{0.0f, -b, -c}
      // clang format on
  };

  constexpr uint32_t indices[] = {
      // clang-format off
      0, 8,  9,
      0, 12, 13,
      0, 16, 17,
      8, 1,  18,
      12, 2,  10,
      16, 4,  14,
      9, 5,  15,
      6, 11, 10,
      3, 19, 18,
      7, 15, 5,
      7,  11, 6,
      7,  19, 3,
      0, 9,  4,
      0, 13, 1,
      0, 17, 2,
      8, 18, 5,
      12, 10, 3,
      16, 14, 6,
      9, 15, 14,
      6, 10, 2,
      3, 18, 1,
      7, 5, 18,
      7,  6,  14,
      7,  3,  10,
      0, 4,  16,
      0, 1,  8,
      0, 2,  12,
      8, 5,  9,
      12, 3,  13,
      16, 6,  17,
      9, 14, 4,
      6, 2,  17,
      3, 1,  13,
      7, 18, 19,
      7,  14, 15,
      7,  10, 11
    // clang-format on
  };

  for (size_t idx = 0; idx < XR_COUNTOF__(vertices); ++idx) {
    mesh->geometry[idx].position = vertices[idx];
  }

  memcpy(base::raw_ptr(mesh->indices), indices, sizeof(indices));
  compute_normals(mesh);
}

void xray::rendering::geometry_factory::icosahedron(geometry_data_t* mesh) {
  constexpr auto t  = 1.618033988749895f;
  constexpr auto it = 0.5257311121191336f;

  const vec3f vertices[] = {vec3f{t, 1.0f, 0.0f} * it,

                            vec3f{-t, 1.0f, 0.0f} * it,

                            vec3f{t, -1.0f, 0.0f} * it,

                            vec3f{-t, -1.0f, 0.0f} * it,

                            vec3f{1.0f, 0.0f, t} * it,

                            vec3f{1.0f, 0.0f, -t} * it,

                            vec3f{-1.0f, 0.0f, t} * it,

                            vec3f{-1.0f, 0.0f, -t} * it,

                            vec3f{0.0f, t, 1.0f} * it,

                            vec3f{0.0f, -t, 1.0f} * it,

                            vec3f{0.0f, t, -1.0f} * it,

                            vec3f{0.0f, -t, -1.0f} * it};

  constexpr uint32_t indices[] = {
    // clang-format off
      0, 8, 4,
      0,  5, 10,
      2, 4, 9,
      2,  11, 5,
      1,  6, 8,
      1, 10, 7,
      3, 9, 6,
      3, 7, 11,
      0, 10, 8,
      1, 8, 10,
      2,  9, 11,
      3, 11, 9,
      4,  2,  0,
      5, 0, 2,
      6, 1,  3,
      7,  3, 1,
      8, 6,  4,
      9, 4,  6,
      10, 5, 7,
      11, 7, 5
    // clang-format on
  };

  mesh->setup(static_cast<uint32_t>(XR_COUNTOF__(vertices)),
              static_cast<uint32_t>(XR_COUNTOF__(indices)));

  for (size_t idx = 0; idx < XR_COUNTOF__(vertices); ++idx) {
    mesh->geometry[idx].position = vertices[idx];
  }

  memcpy(base::raw_ptr(mesh->indices), indices, sizeof(indices));
  compute_normals(mesh);
}

void xray::rendering::geometry_factory::torus(const float      outer_radius,
                                              const float      inner_radius,
                                              const uint32_t   sides,
                                              const uint32_t   rings,
                                              geometry_data_t* mesh) {
  const auto faces      = sides * rings;
  const auto vertex_cnt = sides * (rings + 1);

  mesh->setup(vertex_cnt, 6 * faces);

  const auto ring_factor = two_pi<float> / static_cast<float>(rings);
  const auto side_factor = two_pi<float> / static_cast<float>(sides);

  //
  // vertices
  {
    auto vptr = raw_ptr(mesh->geometry);

    for (uint32_t ring = 0; ring <= rings; ++ring) {
      const auto u  = static_cast<float>(ring) * ring_factor;
      const auto cu = cos(u);
      const auto su = sin(u);

      for (uint32_t side = 0; side < sides; ++side) {
        const auto v  = static_cast<float>(side) * side_factor;
        const auto cv = cos(v);
        const auto sv = sin(v);
        const auto r  = (outer_radius + inner_radius * cv);

        vptr->position  = {r * cu, r * su, inner_radius * sv};
        vptr->normal    = normalize(vec3f{cv * cu * r, cv * su * r, sv * r});
        vptr->texcoords = {u / two_pi<float>, v / two_pi<float>};
        ++vptr;
      }
    }
  }

  //
  // indices
  {
    auto iptr = raw_ptr(mesh->indices);

    for (uint32_t ring = 0; ring < rings; ++ring) {
      const auto ring_start      = ring * sides;
      const auto next_ring_start = (ring + 1) * sides;

      for (uint32_t side = 0; side < sides; ++side) {
        const auto next_side = (side + 1) % sides;

        iptr[0] = ring_start + side;
        iptr[1] = next_ring_start + side;
        iptr[2] = next_ring_start + next_side;

        iptr[3] = ring_start + side;
        iptr[4] = next_ring_start + next_side;
        iptr[5] = ring_start + next_side;

        iptr += 6;
      }
    }
  }
}
