#pragma once

#include <cstdint>
#include <random>
#include <span>

#include "xray/math/scalar2.hpp"
#include "xray/base/memory.arena.hpp"
#include "xray/base/containers/arena.vector.hpp"

//
// Implemented based on this paper:
// https://www.cs.ubc.ca/~rbridson/docs/bridson-siggraph07-poissondisk.pdf
// (this is R^2 only)
class BlueNoisePoissonDiskSampler
{
  public:
    BlueNoisePoissonDiskSampler(xray::base::MemoryArena& arena,
                                const uint32_t width,
                                const uint32_t height,
                                const uint32_t min_points_dist,
                                const uint32_t sample_count);

    void generate_points();
    std::span<const xray::math::vec2ui32> sampled_point() const noexcept { return _samples; }

  private:
    uint32_t add_point_to_grid(const xray::math::vec2ui32 point);
    //
    // generate a random point in the [r; 2r] distance range, around origin_point
    xray::math::vec2ui32 generate_random_point_from_origin(const xray::math::vec2ui32 origin_point);
    //
    // check if a point is valid. For a point to be valid, all its neighbours must be at a distance
    // greater than the specified minimum distance.
    bool is_valid_point(const xray::math::vec2ui32 point);

    inline static constexpr xray::math::vec2ui32 kInvalidPoint{ 0xFFFFFFFF, 0xFFFFFFFF };

    uint32_t _min_dist;
    uint32_t _sample_count;
    uint32_t _width;
    uint32_t _height;
    float _cell_size;
    float _grid_width;
    float _grid_height;
    xray::base::containers::vector<xray::math::vec2ui32> _grid;
    xray::base::containers::vector<uint32_t> _active_points;
    xray::base::containers::vector<xray::math::vec2ui32> _samples;
    std::mt19937 _randgen;
    std::uniform_real_distribution<float> _real_dist;
    std::uniform_real_distribution<float> _angle_dist;
    std::uniform_real_distribution<float> _distance_dist;
    std::uniform_real_distribution<float> _points_sample_dist;
};
