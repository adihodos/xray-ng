#include "xray/xray.hpp"

XR_DISABLE_OPTIMIZATIONS

#include <cstddef>
#include <span>
#include <unordered_set>
#include <fmt/format.h>
#include <fmt/std.h>
#include <boost/ut.hpp>
#include "xray/base/memory.arena.hpp"
#include "xray/math/axis.aligned.bounding.box.r2.hpp"
#include "xray/math/quadtree.hpp"
#include "xray/math/scalar2.hpp"
#include "xray/math/scalar2_string_cast.hpp"
#include "xray/math/scalar2.hash.hpp"

std::byte SCRATCH_BUFFER[64 * 1024 * 1024];

int
main(int argc, char** argv)
{
    using namespace boost::ut::literals;
    using namespace xray::math;
    using namespace xray::base;

    MemoryArena scratch_arena{ std::span{ SCRATCH_BUFFER } };

    BBoxAA2DI32 box{ vec2i32{ -1, -1 }, vec2i32{ 5, 9 } };
    "BoxAA properties"_test = [box]() {
        boost::ut::expect(box.center() == vec2i32{ 2, 4 });
        boost::ut::expect(box.width() == 6);
        boost::ut::expect(box.height() == 10);
        boost::ut::expect(box.extents() == vec2i32{ 3, 5 });
    };

    "BOX union with points"_test = [box]() mutable {
        box |= vec2i32{ -10, 10 };
        boost::ut::expect(box.min == vec2i32{ -10, -1 });
        boost::ut::expect(box.max == vec2i32{ 5, 10 });
    };
    constexpr const vec2i32 grid_points[] = {
        vec2i32{ -32000, -32000 }, vec2i32{ 0, 0 },         vec2i32{ 32000, -32000 },
        vec2i32{ -32000, 32000 },  vec2i32{ 32000, 32000 }, vec2i32{ 256, 523 },
    };

    "BOX from point collection"_test = [&]() {
        constexpr const BBoxAA2DI32 box2{ grid_points };
        boost::ut::expect(box2.min == vec2i32{ -32000, -32000 });
        boost::ut::expect(box2.max == vec2i32{ 32000, 32000 });
        boost::ut::expect(box2.center() == vec2i32{ 0 });
    };

    "QuadTree basic"_test = [&]() {
        constexpr const BBoxAA2DF32 grid{ vec2f32{ -32768.0f }, vec2f32{ 32768.0f } };

        std::unordered_set<vec2i32> prev_frame_nodes{};
        const float pos_frames[] = { 128.0f, -384.0f };

        // for (size_t i = 0; i < 2; ++i) {
        //     ScratchPadArena test_scratchpad{ scratch_arena };
        //
        //     QuadTreeF32 qtree{ scratch_arena, grid.min, grid.max, 256.0f };
        //     qtree.insert(vec2f32{ pos_frames[i] });
        //
        //     std::unordered_set<vec2i32> this_frame_nodes;
        //     for (const auto& node : qtree.get_nodes() | std::views::filter([](const QuadTreeF32::tree_node_type& node) {
        //                                 return node.bbox.width() <= 256.0f;
        //                             })) {
        //         this_frame_nodes.insert(vec2i32{ node.bbox.center() });
        //     }
        //
        //     containers::vector<vec2i32> new_nodes{ MemoryArenaAllocator<vec2i32>{ scratch_arena } };
        //     std::ranges::copy_if(this_frame_nodes, std::back_inserter(new_nodes), [&prev_frame_nodes](vec2i32 n) {
        //         return !prev_frame_nodes.contains(n);
        //     });
        //
        //     containers::vector<vec2i32> despawned_nodes{ MemoryArenaAllocator<vec2i32>{ scratch_arena } };
        //     std::ranges::copy_if(prev_frame_nodes, std::back_inserter(despawned_nodes), [&this_frame_nodes](vec2i32 p) {
        //         return !this_frame_nodes.contains(p);
        //     });
        //
        //     std::ranges::make_heap(despawned_nodes, [O = vec2f32{ pos_frames[i] }](const vec2i32& a, const vec2i32& b) {
        //         const float sqdst_a = squared_distance(O, vec2f32{ a });
        //         const float sqdst_b = squared_distance(O, vec2f32{ b });
        //         return sqdst_a < sqdst_b;
        //     });
        //
        //     fmt::println("\nFrame #{}\nPrevious nodes: ", i);
        //     for (const vec2i32 n : prev_frame_nodes) {
        //         fmt::print("{} ", n);
        //     }
        //
        //     fmt::println("\nframe nodes: ");
        //     for (const vec2i32 n : this_frame_nodes) {
        //         fmt::print("{} ", n);
        //     }
        //
        //     fmt::println("\nSpawned this frame: ");
        //     for (const vec2i32 n : new_nodes) {
        //         fmt::print("{} ", n);
        //     }
        //
        //     fmt::println("\nDespawned this frame: ");
        //     for (const vec2i32 n : despawned_nodes) {
        //         fmt::print("{} ", n);
        //     }
        //     prev_frame_nodes = this_frame_nodes;
        // }
    };
}
