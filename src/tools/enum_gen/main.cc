#include "xray/base/config_settings.hpp"
#include "xray/base/maybe.hpp"
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

std::string replace_all_occurences(const std::string& pattern,
                                   const std::string& replacement,
                                   const std::string& target) {

  std::string::size_type s_beg{};
  std::string::size_type s_end{};
  std::string            result;

  while ((s_end = target.find(pattern, s_beg)) != std::string::npos) {
    assert(s_end >= s_beg);
    result += target.substr(s_beg, s_end - s_beg);
    result += replacement;
    s_beg = s_end + pattern.size();
  }

  result += target.substr(s_beg, std::string::npos);
  return result;
}

struct str_ext {
  static void trim(std::string& s);
  static void trim_start(std::string& s);
  static void trim_end(std::string& s);

  static std::string replace_all(const std::string& pattern,
                                 const std::string& replacement,
                                 const std::string& src);
};

void str_ext::trim_start(std::string& s) {
  s.erase(begin(s), find_if(begin(s), end(s),
                            [](const auto ch) { return !isspace(ch); }));
}

void str_ext::trim_end(std::string& s) {
  auto first_nonspace =
      find_if(rbegin(s), rend(s), [](const auto ch) { return !isspace(ch); });

  if (first_nonspace != rend(s)) {
    s.erase(first_nonspace.base(), end(s));
  }
}

void str_ext::trim(std::string& s) {
  trim_start(s);
  trim_end(s);
}

std::string str_ext::replace_all(const std::string& pattern,
                                 const std::string& replacement,
                                 const std::string& src) {
  std::string::size_type s_beg{};
  std::string::size_type s_end{};
  std::string            result;

  while ((s_end = src.find(pattern, s_beg)) != std::string::npos) {
    assert(s_end >= s_beg);
    result += src.substr(s_beg, s_end - s_beg);
    result += replacement;
    s_beg = s_end + pattern.length();
  }

  result += src.substr(s_beg, std::string::npos);
  return result;
}

void test_stuff() {
  {
    std::string s{"tra la                 "};

    str_ext::trim_end(s);
    assert(s == "tra la");

    str_ext::trim_end(s);
    assert(s == "tra la");
  }

  {
    std::string s{"           tra la "};

    str_ext::trim_start(s);
    assert(s == "tra la ");

    str_ext::trim_start(s);
    assert(s == "tra la ");
  }

  {
    std::string s{"tra la la"};

    str_ext::trim(s);
    assert(s == "tra la la");
  }

  {
    std::string s{"     tra la la"};

    str_ext::trim(s);
    assert(s == "tra la la");
  }

  {
    std::string s{"tra la la      "};

    str_ext::trim(s);
    assert(s == "tra la la");
  }

  {
    std::string s{"     tra la la       "};
    str_ext::trim(s);
    assert(s == "tra la la");
  }
}

int main(int, char**) {
  //  test_stuff();
  //  return 0;

  platformstl::readdir_sequence rss{
      "enumdefs", platformstl::readdir_sequence::files |
                      platformstl::readdir_sequence::fullPath};

  auto file_to_string_fn = [](const char* path) {
    try {
      platformstl::memory_mapped_file mmfile{path};
      return std::string{static_cast<const char*>(mmfile.memory())};
    } catch (const std::exception&) {
    }
    return std::string{};
  };

  const auto template_hpp = file_to_string_fn("enum.hpp.template");
  const auto template_cc  = file_to_string_fn("enum.cc.template");

  for (auto is = begin(rss), ie = end(rss); is != ie; ++is) {
    config_file deffile{*is};

    if (!deffile) {
      printf("\nFailed to open file %s, error %s", *is,
             deffile.error().c_str());
      continue;
    }

    string outtext{template_hpp};

    auto fn_gen_namespaces = [&deffile](string& ns_begin, string& ns_end) {
      const auto ns_entry = deffile.lookup_entry("enum_def.namespaces");
      if (!ns_entry)
        return;

      vector<string> namespaces;
      for (uint32_t i = 0, entries_count = ns_entry.length(); i < entries_count;
           ++i) {
        namespaces.push_back(ns_entry[i].as_string());
      }

      ns_begin = accumulate(begin(namespaces), end(namespaces), std::string(""),
                            [](const string& s, const string& ns) {
                              return s + "namespace " + ns + " { ";
                            });

      ns_end = accumulate(rbegin(namespaces), rend(namespaces), std::string(""),
                          [](const string& s, const string& ns) {
                            return s + "} // namespace " + ns + "\n";
                          });
    };

    {
      string ns_beg, ns_end;
      fn_gen_namespaces(ns_beg, ns_end);

      outtext = replace_all_occurences("{nsbegin}", ns_beg, outtext);
      outtext = replace_all_occurences("{nsend}", ns_end, outtext);

      const char* name{nullptr};
      deffile.lookup_value("enum_def.name", name);

      assert(name != nullptr);

      outtext = replace_all_occurences("{enum_name}", name, outtext);

      const char* underlying_type{nullptr};
      deffile.lookup_value("enum_def.underlying_type", underlying_type);

      outtext = replace_all_occurences(
          "{enum_underlying_type_spec}",
          underlying_type ? fmt::format(": {}", underlying_type) : "", outtext);

      outtext = replace_all_occurences(
          "{enum_underlying_type}",
          underlying_type
              ? underlying_type
              : fmt::format("std::underlying_type<{}::e>::type", name),
          outtext);

      outtext = replace_all_occurences(
          "{type_traits_hdr}", underlying_type ? "" : "#include <type_traits>",
          outtext);

      vector<string> members_with_values;

      {
        const auto mem_sec = deffile.lookup_entry("enum_def.members");
        assert(mem_sec.is_array() && mem_sec.length() >= 1);

        for (uint32_t i = 0; i < mem_sec.length(); ++i) {
          auto memdef = string{mem_sec[i].as_string()};
          members_with_values.push_back(memdef);
        }

        string mws{members_with_values[0]};
        for (size_t i = 1; i < members_with_values.size(); ++i) {
          mws += ",\n";
          mws += members_with_values[i];
        }

        outtext = str_ext::replace_all("{enum_members_and_values}", mws.c_str(),
                                       outtext);

        outtext = str_ext::replace_all(
            "{enum_length}", fmt::format("{}", mem_sec.length()).c_str(),
            outtext);
      }

      {
        for_each(begin(members_with_values), end(members_with_values),
                 [](std::string& s) {
                   str_ext::trim(s);
                   const auto eqpos = s.find('=');
                   if (eqpos == std::string::npos)
                     return;

                   s = s.substr(0, eqpos);
                   str_ext::trim(s);
                 });

        const auto mem_lst = std::accumulate(
            std::next(begin(members_with_values), 1), end(members_with_values),
            fmt::format("e::{}", *begin(members_with_values)),
            [](const std::string& a, const std::string& b) {
              return a + fmt::format(", e::{}", b);
            });

        outtext =
            str_ext::replace_all("{enum_members}", mem_lst.c_str(), outtext);
      }

      {
        bool bitwise{false};
        deffile.lookup_value("enum_def.is_bitfield", bitwise);

        if (bitwise) {
          constexpr auto bitwise_or =
              "constexpr {0}::e operator|(const {0}::e a, const {0}::e b) "
              "noexcept {{"
              "  return static_cast<{0}::e>("
              "      static_cast<{0}::underlying_type>(a) |"
              "      static_cast<{0}::underlying_type>(b));"
              "}}";

          constexpr auto bitwise_and =
              "constexpr {0}::e operator&(const {0}::e a, const {0}::e b) "
              "noexcept {{"
              "  return static_cast<{0}::e>("
              "      static_cast<{0}::underlying_type>(a) &"
              "      static_cast<{0}::underlying_type>(b));"
              "}}";

          outtext = str_ext::replace_all(
              "{bitwise_and}", fmt::format(bitwise_and, name).c_str(), outtext);

          outtext = str_ext::replace_all(
              "{bitwise_or}", fmt::format(bitwise_or, name).c_str(), outtext);
        } else {
          outtext = str_ext::replace_all("{bitwise_and}", "", outtext);
          outtext = str_ext::replace_all("{bitwise_or}", "", outtext);
        }
      }

      {
        std::ofstream outfile{fmt::format("{}.hpp", name)};
        outfile << outtext;
        outfile.flush();
      }

      outtext = template_cc;
      outtext = str_ext::replace_all("{nsbegin}", ns_beg, outtext);
      outtext = str_ext::replace_all("{nsend}", ns_end, outtext);

      outtext = str_ext::replace_all("{enum_file_hpp}",
                                     fmt::format("{}.hpp", name), outtext);
      outtext = str_ext::replace_all("{enum_name}", name, outtext);

      const auto qualified_names_code = accumulate(
          begin(members_with_values), end(members_with_values), std::string{},
          [name](const std::string& a, const std::string& b) {
            return a +
                   fmt::format(
                       "case {1}::e::{0} :\n return \"{1}::e::{0}\";\nbreak;\n",
                       b, name);
          });

      outtext = str_ext::replace_all("{qualified_name_code}",
                                     qualified_names_code, outtext);

      const auto names_code = accumulate(
          begin(members_with_values), end(members_with_values), std::string{},
          [name](const std::string& a, const std::string& b) {
            return a +
                   fmt::format("case {1}::e::{0} :\n return \"{0}\"; break;\n",
                               b, name);
          });

      outtext =
          str_ext::replace_all("{unqualified_name_code}", names_code, outtext);

      {
        std::ofstream outfile{fmt::format("{}.cc", name)};
        outfile << outtext;
        outfile.flush();
      }
    }
  }

  return EXIT_SUCCESS;
}
