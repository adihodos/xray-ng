#pragma once

#include <cstdint>

#include <tl/expected.hpp>
#include <noise/noise.h>
#include <noise/noiseutils.h>

#include "xray/base/memory.arena.hpp"
#include "xray/base/unique_pointer.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.error.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.bindless.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.buffer.hpp"
#include "xray/rendering/geometry/procedural.terrain.hpp"

namespace xray::ui {
class user_interface;
};

namespace B5 {

struct RenderEvent;
struct InitContext;

struct NoiseGen
{
    noise::module::RidgedMulti ridged;
    noise::module::Billow base_flat_terrain;
    noise::module::ScaleBias flat_terrain;
    noise::module::Perlin terrain_type;
    noise::module::Select terrain_selector;
    noise::module::Turbulence final_terrain;
    noise::utils::NoiseMap heightmap;
    noise::utils::NoiseMapBuilderPlane heightmap_builder;
};

class Terrain
{
  public:
    static tl::expected<Terrain, xray::rendering::VulkanError> create(const InitContext& ctx);

  private:
    struct PrivateConstructionToken
    {
        explicit PrivateConstructionToken() = default;
    };

    xray::base::unique_pointer<NoiseGen> _noise_gen;
    xray::rendering::TerrainParams _terrain_params;

    struct RenderResources
    {
        xray::rendering::VulkanBuffer vertexbuffer;
        xray::rendering::VulkanBuffer indexbuffer;
        xray::rendering::BindlessStorageBufferResourceHandleEntryPair instances;
        xray::rendering::BindlessImageResourceHandleEntryPair heightmap;
        xray::rendering::BindlessImageResourceHandleEntryPair colormap;
        uint32_t index_count{};
    } _renderstate;

  public:
    Terrain(PrivateConstructionToken,
            xray::rendering::VulkanBuffer vertexbuffer,
            xray::rendering::VulkanBuffer indexbuffer,
            xray::rendering::BindlessStorageBufferResourceHandleEntryPair instances,
            xray::rendering::BindlessImageResourceHandleEntryPair hmap,
            xray::rendering::BindlessImageResourceHandleEntryPair cmap,
            uint32_t index_count,
            xray::base::unique_pointer<NoiseGen> noise_gen,
            xray::rendering::TerrainParams terrain_params);

    Terrain(Terrain&&) = default;

    void loop_event(const RenderEvent& re);
    void user_interface(xray::ui::user_interface* ui, const RenderEvent& re);
};

}
