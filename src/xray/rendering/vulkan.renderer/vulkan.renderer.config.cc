#include "xray/rendering/vulkan.renderer/vulkan.renderer.config.hpp"

#include "xray/base/xray.stringview.hpp"
#include "xray/base/serialization/serialization.hpp"

//
// TODO: doesnt look like this class has much purpose ...
xray::rendering::RendererConfig xray::rendering::RendererConfig::from_file(
	xray::base::MemoryArena& arena, const xray::base::xrStringView_t file_path
) {
	RendererConfig config{};
	base::deserialize_from_file(arena, config, file_path);
	return config;
}

void xray::rendering::RendererConfig::WriteToFile(const xray::base::xrStringView_t path) {
	base::serialize_to_file(*this, path);
}
