#include "xray/base/unique_pointer.hpp"
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fmt/format.h>
#include <fstream>
#include <iostream>
#include <memory>
#include <numeric>
#include <platformstl/filesystem/current_directory.hpp>
#include <platformstl/filesystem/directory_functions.hpp>
#include <platformstl/filesystem/memory_mapped_file.hpp>
#include <platformstl/filesystem/path.hpp>
#include <platformstl/filesystem/readdir_sequence.hpp>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

using namespace std;
using namespace xray::base;

std::string
replace_all_occurences(const std::string& pattern, const std::string& replacement, const std::string& target)
{

    std::string::size_type s_beg{};
    std::string::size_type s_end{};
    std::string result;

    while ((s_end = target.find(pattern, s_beg)) != std::string::npos) {
        assert(s_end >= s_beg);
        result += target.substr(s_beg, s_end - s_beg);
        result += replacement;
        s_beg = s_end + pattern.size();
    }

    result += target.substr(s_beg, std::string::npos);
    return result;
}

struct str_ext
{
    static void trim(std::string& s);
    static void trim_start(std::string& s);
    static void trim_end(std::string& s);

    static std::string replace_all(const std::string& pattern, const std::string& replacement, const std::string& src);
};

void
str_ext::trim_start(std::string& s)
{
    s.erase(begin(s), find_if(begin(s), end(s), [](const auto ch) { return !isspace(ch); }));
}

void
str_ext::trim_end(std::string& s)
{
    auto first_nonspace = find_if(rbegin(s), rend(s), [](const auto ch) { return !isspace(ch); });

    if (first_nonspace != rend(s)) {
        s.erase(first_nonspace.base(), end(s));
    }
}

void
str_ext::trim(std::string& s)
{
    trim_start(s);
    trim_end(s);
}

std::string
str_ext::replace_all(const std::string& pattern, const std::string& replacement, const std::string& src)
{
    std::string::size_type s_beg{};
    std::string::size_type s_end{};
    std::string result;

    while ((s_end = src.find(pattern, s_beg)) != std::string::npos) {
        assert(s_end >= s_beg);
        result += src.substr(s_beg, s_end - s_beg);
        result += replacement;
        s_beg = s_end + pattern.length();
    }

    result += src.substr(s_beg, std::string::npos);
    return result;
}

using namespace std;
using namespace xray::base;

int
main(int, char**)
{
    //  test_stuff();
    //  return 0;
    struct key_map_info
    {
        const char* file_name;
        const char* table_name;
        const uint32_t mask;
    } static constexpr mappings[] = { { "symtables/x11.keymap.0", "X11_MISC_FUNCTION_KEYS_MAPPING_TABLE", 0xFF },
                                      { "symtables/x11.keymap.1", "X11_LATIN1_KEYS_MAPPING_TABLE", 0xFF00 } };

    vector<string> gen_code;

    auto keymapfn = [](const key_map_info& kmi) {
        unique_pointer<FILE, decltype(&fclose)> fp{ fopen(kmi.file_name, "r"), &fclose };

        assert(static_cast<bool>(fp));

        const char* ptr = (const char*)mp.data();
    };

    transform(begin(mappings), end(mappings), back_inserter(gen_code), [](const auto& mp) {

    });

    return EXIT_SUCCESS;
}
