#pragma once

#include <filesystem>
#include <cstdint>

namespace xray::rendering {

struct RendererConfig
{
    bool validate_core{ true };
    bool validate_sync{ true };
    bool thread_safety_checks{ true };
    bool enable_message_limit{ true };
    int32_t max_duplicate_messages{ 4 };

    static RendererConfig from_file(const std::filesystem::path& file_path);
    void WriteToFile(const std::filesystem::path& path);
};

}
