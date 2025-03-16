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
    float xmargin{ 256.0f };
    float ymargin{ 0.0f };
    float angle_increment{ 15.0f };
    float arc_degrees{ 120.0f };
    float label_angle_multiple{ 2.0f };
    std::string glyph_major{ "|" };
    std::string glyph_minor{ "-" };
};

struct RangedIndicatorDefinition
{
    float xmargin{ 256.0f };
    float ymargin{ 128.0f };
    float range{ 300.0f };
    float increment{ 10.0f };
    float bar_ends_len{ 32.0f };
    float bar_width{ 8.0f };
    float marker_big_meters{ 50.0f };
    float marker_big_len{ 8.0f };
    float marker_small_len{ 4.0f };
    float marker_height{ 4.0f };
    float dir_x{0.0};
    float dir_y{1.0};
    float marker_dir_x{-1.0};
    float marker_dir_y{0.0};
};

struct HudAltimeterDefinition
{
    float xmargin{ 256.0f };
    float ymargin{ 128.0f };
    float range{ 300.0f };
    float increment{ 10.0f };
    float bar_ends_len{ 32.0f };
    float bar_width{ 8.0f };
    float marker_big_meters{ 50.0f };
    float marker_big_len{ 8.0f };
    float marker_small_len{ 4.0f };
    float marker_height{ 4.0f };
};

struct HudSpeedometerDefinition
{
    float xmargin{ 256.0f };
    float ymargin{ 128.0f };
    float range{ 300.0f };
    float increment{ 10.0f };
    float bar_ends_len{ 32.0f };
    float bar_width{ 8.0f };
    float marker_big_meters{ 50.0f };
    float marker_big_len{ 8.0f };
    float marker_small_len{ 4.0f };
    float marker_height{ 4.0f };
};

struct HudConfigDefinition
{
    std::vector<HudFontDefinition> font_list;
    HudCompassDefinition compass;
    // HudAltimeterDefinition altimeter;
    RangedIndicatorDefinition speedometer;
    RangedIndicatorDefinition altimeter;
    // std::vector<HudConfigurationTextElement> text_elements;
};

}
