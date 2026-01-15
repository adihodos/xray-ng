#include "blue.noise.poisson.disk.sampler.hpp"
#include <cassert>
#include <cmath>
#include <numbers>

#include "xray/math/scalar2_math.hpp"

XR_DISABLE_OPTIMIZATIONS()

BlueNoisePoissonDiskSampler::BlueNoisePoissonDiskSampler(xray::base::MemoryArena& arena,
                                                         const uint32_t width,
                                                         const uint32_t height,
                                                         const uint32_t min_points_dist,
                                                         const uint32_t sample_count)

    : _min_dist{ min_points_dist }
    , _sample_count{ sample_count }
    , _width{ width }
    , _height{ height }
    , _cell_size{ std::sqrt(static_cast<float>(min_points_dist) * 0.5f) }
    , _grid_width{ std::ceil(_width / _cell_size) + 1.0f }
    , _grid_height{ std::ceil(_height / _cell_size) + 1.0f }
    , _grid{ static_cast<size_t>(_grid_width * _grid_height), kInvalidPoint, arena }
    , _active_points{ arena }
    , _samples{ arena }
    , _randgen{ std::random_device{}() }
    , _real_dist{ 0.0f, 1.0f }
    , _angle_dist{ 0.0f, 2.0f * std::numbers::pi }
    , _distance_dist{ static_cast<float>(_min_dist), 2.0f * static_cast<float>(_min_dist) }
    , _points_sample_dist{ 0.0f, 1.0f }
{
    //
    // Generate an initial random point and add it to the grid and the list of active points.
    const xray::math::vec2ui32 initial_point{
        static_cast<uint32_t>(_real_dist(_randgen) * static_cast<float>(_width)),
        static_cast<uint32_t>(_real_dist(_randgen) * static_cast<float>(_height)),
    };

    add_point_to_grid(initial_point);
    _active_points.push_back(0);
}

uint32_t
BlueNoisePoissonDiskSampler::add_point_to_grid(const xray::math::vec2ui32 point)
{
    const float x_coord = std::floor(static_cast<float>(point.x) / _cell_size);
    const float y_coord = std::floor(static_cast<float>(point.y) / _cell_size);

    const uint32_t cell_idx = static_cast<uint32_t>(y_coord * _cell_size + x_coord);
    _grid[cell_idx] = point;
    return cell_idx;
}

xray::math::vec2ui32
BlueNoisePoissonDiskSampler::generate_random_point_from_origin(const xray::math::vec2ui32 origin_point)
{
    //
    // generate random point around origin point, using polar coords
    const float rand_angle = _angle_dist(_randgen);
    const float rand_radius = _distance_dist(_randgen);

    const xray::math::vec2f32 rand_point =
        xray::math::vec2f32{ origin_point } +
        xray::math::vec2f32{ rand_radius * std::cos(rand_angle), rand_radius * std::sin(rand_angle) };

    //
    // constrain point to grid bounds
    return xray::math::vec2ui32{
        static_cast<uint32_t>(xray::math::clamp(rand_point.x, 0.0f, static_cast<float>(_width) - 1.0f)),
        static_cast<uint32_t>(xray::math::clamp(rand_point.y, 0.0f, static_cast<float>(_height) - 1.0f)),
    };
}

bool
BlueNoisePoissonDiskSampler::is_valid_point(const xray::math::vec2ui32 point)
{
    const float xcoord = std::floor(static_cast<float>(point.x) / _cell_size);
    const float ycoord = std::floor(static_cast<float>(point.y) / _cell_size);

    const uint32_t start_x = static_cast<uint32_t>(std::max(0.0f, xcoord - 2.0f));
    const uint32_t end_x = static_cast<uint32_t>(std::min(xcoord + 2.0f, _grid_width - 1.0f));
    const uint32_t start_y = static_cast<uint32_t>(std::max(0.0f, ycoord - 2.0f));
    const uint32_t end_y = static_cast<uint32_t>(std::min(ycoord + 2.0f, _grid_height - 1.0f));

    for (uint32_t y = start_y; y < end_y; ++y) {
        for (uint32_t x = start_x; x < end_x; ++x) {
            const uint32_t idx = y * static_cast<uint32_t>(_grid_width) + x;
            if (const xray::math::vec2ui32 current_point = _grid[idx]; current_point != kInvalidPoint) {
                if (xray::math::squared_distance(current_point, point) <= _min_dist * _min_dist) {
                    return false;
                }
            }
        }
    }

    return true;
}

void
BlueNoisePoissonDiskSampler::generate_points()
{
    while (!_active_points.empty()) {
        //
        // get a random point from the active points list
        const uint32_t point_idx = _points_sample_dist(_randgen) * static_cast<float>(_active_points.size() - 1);
        const xray::math::vec2ui32 chosen_point = _grid[_active_points[point_idx]];

        assert(chosen_point != kInvalidPoint);
        //
        // number of randomly generated points around chosen_point that are valid
        uint32_t added_points_count{ 0 };

        //
        // generate sample_count points around the current active point
        for (uint32_t sample = 0; sample < _sample_count; ++sample) {
            const xray::math::vec2ui32 rand_point = generate_random_point_from_origin(chosen_point);
            //
            // If the generated point is valid, add it to the list
            if (is_valid_point(rand_point)) {
                const uint32_t insertion_index = add_point_to_grid(rand_point);
                _active_points.push_back(insertion_index);
                _samples.push_back(rand_point);
                ++added_points_count;
            }
        }

        //
        // if the point fails to produce valid neighbours, we remove it from the active point list.
        if (!added_points_count) {
            _active_points.erase(std::next(_active_points.begin(), point_idx));
        }
    }
}
