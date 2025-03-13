#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <tuple>
#include <filesystem>

namespace B5 {
struct HudFontDefinition
{
    std::filesystem::path path;
    std::vector<uint8_t> sizes;
    std::vector<std::tuple<uint16_t, uint16_t>> glyph_ranges;
};

struct HudCompassDefinition
{
    float ypos{ 32.0f };
    float bar_width{ 1200.0f };
    float angle_increment{ 15.0f };
    float arc_degrees{ 120.0f };
    std::string glyph_major{ "|" };
    std::string glyph_minor{ "-" };
};

struct HudConfigDefinition
{
    std::vector<HudFontDefinition> font_list;
    HudCompassDefinition compass;
    // std::vector<HudConfigurationTextElement> text_elements;
};

}
