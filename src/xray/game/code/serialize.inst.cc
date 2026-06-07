#include "xray/base/serialization/serialization.hpp"
#include "xray/base/serialization/serialization.cc"

#include "xray/scene/scene.description.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.renderer.config.hpp"
#include "xray/rendering/sprite.system/sprite.atlas.hpp"

#include "flight.cam.hpp"

// Explicit template instantiations for serialization / deserialization

template bool xray::base::deserialize_from_file(
	xray::base::MemoryArena&, xray::scene::SceneDescription&, const xray::base::xrStringView_t
);

template bool xray::base::deserialize_from_file(
	xray::base::MemoryArena&, xray::rendering::RendererConfig&, xray::base::xrStringView_t
);
template bool xray::base::deserialize_from_file(
	xray::base::MemoryArena&, B5::FlightCameraParams&, xray::base::xrStringView_t
);
template bool xray::base::deserialize_from_file(
	xray::base::MemoryArena&, xray::rendering::TextureAtlasData&, xray::base::xrStringView_t
);
template xray::base::xrSerializeResult_t xray::base::serialize_to_file(
	xray::rendering::RendererConfig const&, xray::base::xrStringView_t
);
