//
// Copyright (c) 2011, 2012, 2013 Adrian Hodos
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above copyright
//       notice, this list of conditions and the following disclaimer in the
//       documentation and/or other materials provided with the distribution.
//     * Neither the name of the author nor the
//       names of its contributors may be used to endorse or promote products
//       derived from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR THE CONTRIBUTORS BE LIABLE FOR
// ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#pragma once

#include <cassert>
#include <cstdint>
#include <algorithm>
#include <ranges>
#include <vector>
#include <span>
#include <optional>

#include "xray/base/memory.arena.hpp"
#include "xray/base/containers/arena.vector.hpp"
#include "xray/math/math_std.hpp"
#include "xray/math/concept.point.hpp"
#include "xray/math/axis.aligned.bounding.box.r2.hpp"
#include "xray/math/scalar2_math.hpp"

namespace xray {
namespace math {

/// \addtogroup __GroupXrayMath_Geometry
/// @{

template<typename PointType>
    requires Point<PointType> && requires { Rank<PointType>::R == 2; }
struct QuadTreeNode
{
    BoundingBoxAxisAligned<PointType> bbox;
    std::optional<uint32_t> links[4]{};

    bool is_leaf_node() const noexcept { return !links[0]; }
};

///
/// based on this
/// https://lisyarus.github.io/blog/posts/building-a-quadtree.html

template<typename PointType>
    requires Point<PointType> && requires { Rank<PointType>::R == 2; }
struct QuadTree
{
    using tree_node_type = QuadTreeNode<PointType>;
    using value_type = typename PointType::value_type;
    using bbox_type = BBoxAA2D<value_type>;

    static constexpr const value_type MIN_SQUARED_DST{ value_type{ 512 } * value_type{ 512 } };

    value_type min_node_size_squared{ 1024 };
    value_type max_visiblity_distance_squared{ 1024 };
    std::optional<uint32_t> root;
    xray::base::containers::vector<tree_node_type> nodes;

    QuadTree(base::MemoryArena& arena,
             const bbox_type& bounds,
             const value_type min_dst_to_child,
             const value_type maximum_visible_distance)
        : QuadTree{ arena, bounds.min, bounds.max, min_dst_to_child, maximum_visible_distance }
    {
    }

    QuadTree(base::MemoryArena& arena,
             const PointType& minp,
             const PointType& maxp,
             const value_type minimum_node_size,
             const value_type maximum_visible_distance)
        : min_node_size_squared{ minimum_node_size * minimum_node_size }
        , max_visiblity_distance_squared{ maximum_visible_distance * maximum_visible_distance }
        , root{ 0 }
        , nodes{ base::MemoryArenaAllocator<tree_node_type>{ arena } }
    {
        nodes.push_back(tree_node_type{ bbox_type{ minp, maxp } });
    }

    std::span<const tree_node_type> get_nodes() const noexcept
    {
        assert(!nodes.empty());
        return std::span{ nodes }.subspan(1, nodes.size() - 1);
    }

    value_type squared_distance_point_to_node(uint32_t child, const PointType pos) const noexcept
    {
        assert(size_t{ child } < nodes.size());
        return squared_distance(nodes[child].bbox.center(), pos);
    }

    void insert_impl(uint32_t node_id, const PointType pos)
    {
        const value_type squared_distance_to_node = squared_distance_point_to_node(node_id, pos);
        const value_type squared_node_width = nodes[node_id].bbox.width() * nodes[node_id].bbox.width();

        if (squared_distance_to_node < max_visiblity_distance_squared && squared_node_width > min_node_size_squared) {
            const PointType center = nodes[node_id].bbox.center();

            //
            // bottom left
            if (!nodes[node_id].links[0]) {
                const uint32_t id = static_cast<uint32_t>(nodes.size());
                nodes.push_back(tree_node_type{ bbox_type{ nodes[node_id].bbox.min, center } });
                nodes[node_id].links[0] = id;
            }

            if (!nodes[node_id].links[1]) {
                const uint32_t id = static_cast<uint32_t>(nodes.size());
                nodes.push_back(tree_node_type{ bbox_type{ PointType{ center.x, nodes[node_id].bbox.min.y },
                                                           PointType{ nodes[node_id].bbox.max.x, center.y } } });
                nodes[node_id].links[1] = id;
            }

            if (!nodes[node_id].links[2]) {
                const uint32_t id = static_cast<uint32_t>(nodes.size());
                nodes.push_back(tree_node_type{ bbox_type{ PointType{ nodes[node_id].bbox.min.x, center.y },
                                                           PointType{ center.x, nodes[node_id].bbox.max.y } } });
                nodes[node_id].links[2] = id;
            }

            if (!nodes[node_id].links[3]) {
                const uint32_t id = static_cast<uint32_t>(nodes.size());
                nodes.push_back(tree_node_type{ bbox_type{ center, nodes[node_id].bbox.max } });
                nodes[node_id].links[3] = id;
            }

            for (size_t i = 0; i < 4; ++i) {
                insert_impl(*nodes[node_id].links[i], pos);
            }
        }
    }

    void insert(const PointType pos) { insert_impl(*root, pos); }

    const tree_node_type& get_node(uint32_t id) const noexcept
    {
        assert(id < nodes.size());
        return nodes[id];
    }

    size_t node_count() const noexcept { return nodes.size(); }
};

using QuadTreeF32 = QuadTree<vec2f32>;

/// @}

}
}
