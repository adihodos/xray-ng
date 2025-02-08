#pragma once

#include <cstdint>
#include <span>
#include <xray/rendering/vulkan.renderer/vulkan.bindless.hpp>

namespace B5 {

struct PushConstantPacker
{
    union
    {
        uint32_t value;
        struct
        {
            uint32_t frame_id : 8;
            uint32_t instance_entry : 8;
            uint32_t instance_buffer_id : 16;
        };
    };

    PushConstantPacker(const xray::rendering::BindlessResourceHandle_StorageBuffer sbo,
                       const uint32_t instance_entry,
                       const uint32_t frame) noexcept
    {
        this->instance_buffer_id = destructure_bindless_resource_handle(sbo).first;
        this->instance_entry = instance_entry;
        this->frame_id = frame;
    }

    PushConstantPacker(const uint32_t frame) noexcept
    {
        this->instance_buffer_id = this->instance_entry = 0;
        this->frame_id = frame;
    }

    std::span<const uint8_t> as_bytes() const noexcept
    {
        return std::span{ reinterpret_cast<const uint8_t*>(&value), 4 };
    }
};

}
