#pragma once

#include <cstdint>
#include <fmt/format.h>
#include <vulkan/vulkan_core.h>

template<>
struct fmt::formatter<VkExtent2D> : fmt::nested_formatter<uint32_t>
{
    auto format(VkExtent2D p, format_context& ctx) const
    {
        return this->write_padded(ctx, [this, p](auto out) {
            return fmt::format_to(out, "VkExtent2D = {{.width = {}, .height = {}}}", nested(p.width), nested(p.height));
        });
    }
};

template<>
struct fmt::formatter<VkOffset2D> : fmt::nested_formatter<int32_t>
{
    auto format(VkOffset2D p, format_context& ctx) const
    {
        return this->write_padded(ctx, [this, p](auto out) {
            return fmt::format_to(out, "VkOffset2D = {{.x = {}, .y = {}}}", nested(p.x), nested(p.y));
        });
    }
};

template<>
struct fmt::formatter<VkExtent3D> : fmt::nested_formatter<uint32_t>
{
    auto format(VkExtent3D p, format_context& ctx) const
    {
        return this->write_padded(ctx, [this, p](auto out) {
            return fmt::format_to(out,
                                  "VkExtent3D = {{.width = {}, .height = {}, .depth = {}}}",
                                  nested(p.width),
                                  nested(p.height),
                                  nested(p.depth));
        });
    }
};

template<>
struct fmt::formatter<VkOffset3D> : fmt::nested_formatter<int32_t>
{
    auto format(VkOffset3D p, format_context& ctx) const
    {
        return this->write_padded(ctx, [this, p](auto out) {
            return fmt::format_to(
                out, "VkOffset3D = {{.x = {}, .y = {}, .z = {}}}", nested(p.x), nested(p.y), nested(p.z));
        });
    }
};

template<>
struct fmt::formatter<VkSurfaceCapabilitiesKHR>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    template<typename Context>
    constexpr auto format(const VkSurfaceCapabilitiesKHR& caps, Context& ctx) const
    {
        return fmt::format_to(
            ctx.out(),
            "VkSurfaceCapabilitiesKHR = {{\n\t.minImageCount = {},\n\t.maxImageCount = {},\n\t.currentExtent = "
            "{},\n\t.minImageExtent = {},\n\t.maxImageExtent = {}\n}}",
            caps.minImageCount,
            caps.maxImageCount,
            caps.currentExtent,
            caps.minImageExtent,
            caps.maxImageExtent);
    }
};

template<>
struct fmt::formatter<VkDrawIndexedIndirectCommand>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    template<typename Context>
    constexpr auto format(const VkDrawIndexedIndirectCommand& caps, Context& ctx) const
    {
        return fmt::format_to(
            ctx.out(),
            "VkDrawIndexedIndirectCommand = {{\n\t.indexCount = {},\n\t.instanceCount = {},\n\t.firstIndex = "
            "{},\n\t.vertexOffset = {},\n\t.firstInstance = {}\n}}",
            caps.indexCount,
            caps.instanceCount,
            caps.firstIndex,
            caps.vertexOffset,
            caps.firstInstance);
    }
};
