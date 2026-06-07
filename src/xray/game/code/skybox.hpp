#pragma once

namespace B5 {

class SkyBox {
private:
	struct PrivateConstructionToken {
		explicit PrivateConstructionToken() = default;
	};

public:
	SkyBox();

private:
	xray::rendering::BindlessImageResourceHandleEntryPair _skybox;
	xray::rendering::VulkanPipeline _p_skybox;
};
}  // namespace B5
