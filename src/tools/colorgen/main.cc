#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstdlib>

#include <algorithm>
#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <platformstl/filesystem/memory_mapped_file.hpp>

using color_defs_collection = std::unordered_map<std::string, uint32_t>;

color_defs_collection
process_color_def_file(const char* file_name)
{
    assert(file_name != nullptr);

    color_defs_collection color_table;

    platformstl::memory_mapped_file mm_file{ file_name };
    auto p = static_cast<const char*>(mm_file.memory());

    uint32_t line{};

    std::vector<char> color_name;
    std::vector<char> color_value;

    for (;;) {
        if (!*p)
            break;

        if (*p == '\n') {
            ++line;
            ++p;
            continue;
        }

        if (*p == '/') {
            if (!*(p + 1))
                break;

            if (*(p + 1) != '*')
                break;

            p += 2;

            while (*p) {

                if (*p == '*') {
                    if (*(p + 1) == '/') {
                        p += 2;
                        break;
                    }
                }
                ++p;
            }

            continue;
        }

        if (*p == '.') {
            ++p;

            assert(color_name.empty());
            assert(color_value.empty());

            while (*p && isspace(*p))
                ++p;

            while (*p && (isalnum(*p) || (*p == '-'))) {
                color_name.push_back(*p++);
            }

            if (!color_name.empty()) {
                color_name.push_back(0);
            }

            continue;
        }

        if (*p == '#') {
            ++p;

            assert(!color_name.empty());
            assert(color_value.empty());

            while (*p && isspace(*p))
                ++p;

            auto is_hex_letter = [](const char p) { return (p >= 'a' && p <= 'f') || (p >= 'A' && p <= 'F'); };

            while (*p && (is_hex_letter(*p) || isdigit(*p))) {
                color_value.push_back(*p++);
            }

            if (!color_value.empty()) {
                color_value.push_back(0);

                try {

                    const auto clr_val = std::stoul(static_cast<const char*>(&color_value[0]), 0, 16);

                    color_table[static_cast<const char*>(&color_name[0])] = clr_val;

                } catch (const std::exception&) {
                }
            }

            color_name.clear();
            color_value.clear();

            continue;
        }

        ++p;
    }

    return color_table;
}

void
write_color_defs(const char* defs_file, const color_defs_collection& coll)
{
    const std::string fname{ defs_file };

    const auto name = fname.substr(0, fname.length() - fname.find('.'));

    std::cout << "colors [ " << name << " ] " << std::endl;

    for (const auto& kvp : coll)
        std::cout << kvp.first << " " << std::hex << kvp.second << std::endl;
}

int
main(int, char**)
{
    constexpr const char* css_files[] = { "colordefs/flat_colors.css" };

    using namespace std;
    for_each(begin(css_files), end(css_files), [](const char* file_name) {
        try {
            const auto color_tbl = process_color_def_file(file_name);
            write_color_defs(file_name, color_tbl);

        } catch (const std::exception& e) {
            cerr << "Error processing file " << file_name << " " << e.what() << endl;
            return;
        }
    });

    return 0;
}
