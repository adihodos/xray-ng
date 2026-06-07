#pragma once

#include "xray/xray.hpp"
#include "xray/base/xray.stringview.hpp"

namespace xray::base {
struct MemoryArena;
}

namespace xray::rendering {

struct RendererConfig {
	bool validate_core{true};
	bool validate_sync{true};
	bool thread_safety_checks{true};
	bool enable_message_limit{true};
	I32 max_duplicate_messages{4};

	static RendererConfig from_file(xray::base::MemoryArena& arena, const xray::base::xrStringView_t file_path);
	void WriteToFile(const xray::base::xrStringView_t path);
};

}  // namespace xray::rendering
