#include "xray/rendering/vulkan.renderer/vulkan.renderer.config.hpp"
#include "xray/base/serialization/rfl.libconfig/config.save.hpp"
#include "xray/base/serialization/rfl.libconfig/config.load.hpp"

xray::rendering::RendererConfig
xray::rendering::RendererConfig::from_file(const std::filesystem::path& file_path)
{
    RendererConfig config{};
    const rfl::Result<RendererConfig> loaded_config = rfl::libconfig::read<RendererConfig>(file_path);

    if (loaded_config) {
        config = loaded_config.value();
    }

    return config;
}

void
xray::rendering::RendererConfig::WriteToFile(const std::filesystem::path& path)
{
    rfl::libconfig::save(path.generic_string(), *this);
}
