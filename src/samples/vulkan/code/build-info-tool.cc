#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <memory>
#include <chrono>
#include <sstream>
#include <string>
#include <string_view>
#include <optional>
#include <ranges>

#include <fmt/core.h>
#include <fmt/chrono.h>

#if defined(PLATFORM_WINDOWS)
#define SECURITY_WIN32
#include <windows.h>
#include <security.h>
#include <winsock.h>
#else
#include <git2.h>
#include <pwd.h>
#include <unistd.h>
#endif

using namespace std;
namespace fs = std::filesystem;

struct FileDeleter
{
    void operator()(FILE* f) const noexcept
    {
        if (f)
            fclose(f);
    }
};

int32_t
write_app_config(const fs::path& output_file, const fs::path& shader_redirect_dir)
{
    fmt::print(stderr, "\n{}", shader_redirect_dir.generic_string());

    static constexpr const char* const cfg_ovr = R"(
directories : {{ 
    root_win = "d:/games/xray"; 
    root = "/home/adi/games/xray";
    models = "assets/models"; 
    fonts = "assets/fonts";
    textures = "assets/textures"; 
    shaders = "{shader_redirect_dir}";
}};
    )";

    // fmt::print("::: Writing custom file: {} :::", output_file.generic_string());
    unique_ptr<FILE, FileDeleter> out_file{ fopen(output_file.generic_string().c_str(), "wt") };
    if (!out_file) {
        fmt::print(stderr, "\nFailed to create build config file {}", output_file.generic_string());
        return EXIT_FAILURE;
    }

    fmt::print(out_file.get(), cfg_ovr, fmt::arg("shader_redirect_dir", shader_redirect_dir.generic_string()));
    return EXIT_SUCCESS;
}

int32_t
write_build_info(const fs::path& output_dir)
{
#if defined(PLATFORM_LINUX)
    if (git_libgit2_init() < 0) {
        return EXIT_FAILURE;
    }

    const fs::path repo_lookup_path{ fs::current_path() };

    git_buf repo_root{};
    if (git_repository_discover(&repo_root, repo_lookup_path.generic_string().c_str(), true, nullptr) != 0) {
        fmt::print(stderr, "Failed to discover repo root, started from: {}", repo_lookup_path.generic_string().c_str());
        return EXIT_FAILURE;
    }

    git_repository* repo{ nullptr };
    const int result = git_repository_open(&repo, repo_root.ptr);
    if (result < 0) {
        fmt::print(stderr, "Failed to open repository from path {}\n", repo_lookup_path.generic_string());
        return EXIT_FAILURE;
    }

    git_oid oid_parent_commit{};
    if (git_reference_name_to_id(&oid_parent_commit, repo, "HEAD") != 0) {
        return EXIT_FAILURE;
    }

    char git_commit_sha1[GIT_OID_MAX_SIZE + 1]{};
    git_oid_tostr(git_commit_sha1, size(git_commit_sha1), &oid_parent_commit);
    optional<string> commit_sha1{ string{ git_commit_sha1 } };

#else
    optional<string> commit_sha1 = []() -> optional<string> {
        SECURITY_ATTRIBUTES sec_attrs{};
        sec_attrs.nLength = sizeof(sec_attrs);
        sec_attrs.bInheritHandle = true;

        HANDLE read_pipe{};
        HANDLE write_pipe{};
        if (!CreatePipe(&read_pipe, &write_pipe, &sec_attrs, 0)) {
            return nullopt;
        }

        SetHandleInformation(read_pipe, HANDLE_FLAG_INHERIT, 0);

        STARTUPINFOA si{};
        si.cb = sizeof(si);
        si.hStdOutput = write_pipe;
        si.hStdError = write_pipe;
        si.dwFlags = STARTF_USESTDHANDLES;

        PROCESS_INFORMATION pi{};

        char command_line[] = "git describe --match=NeVeRmAtCh --always --abbrev=40 --dirty";
        if (!CreateProcessA(
                nullptr, command_line, nullptr, nullptr, true, CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi)) {
            return nullopt;
        }

        const auto wait_result = WaitForSingleObjectEx(pi.hProcess, 4000, false);
        if (wait_result != WAIT_OBJECT_0) {
            return nullopt;
        }

        DWORD exit_code{ EXIT_FAILURE };
        GetExitCodeProcess(pi.hProcess, &exit_code);
        if (exit_code != EXIT_SUCCESS) {
            return nullopt;
        }

        char text_buffer[2048];
        DWORD bytes_read{};
        const auto read_ok =
            ReadFile(read_pipe, text_buffer, static_cast<DWORD>(size(text_buffer) - 1), &bytes_read, nullptr);

        if (!read_ok || bytes_read == 0)
            return nullopt;

        text_buffer[bytes_read] = 0;
        string s{};
        ranges::copy(span{ text_buffer, size_t{ bytes_read } } | views::filter([](char c) { return !isspace(c); }),
                     back_inserter(s));

        return optional<string>{ s };
    }();
#endif

    const string user_info = []() {
#if defined(PLATFORM_WINDOWS)
        char temp_buffer[2048]{};
        ULONG max_chars{ 2047 };
        const auto result = GetUserNameExA(NameSamCompatible, temp_buffer, &max_chars);
        return fmt::format("{}", result ? temp_buffer : "unknown");
#else
        vector<uint8_t> temp_buffer;
        const uid_t user_id = getuid();
        const auto bsize = sysconf(_SC_GETPW_R_SIZE_MAX);
        temp_buffer.resize(bsize > 0 ? static_cast<size_t>(bsize) : 16384);

        passwd user_data;
        passwd* result{};
        getpwuid_r(user_id, &user_data, reinterpret_cast<char*>(temp_buffer.data()), temp_buffer.size(), &result);
        return fmt::format(
            "{} ({}, uid {})", result ? result->pw_name : "unknown", result ? result->pw_gecos : "unknown", user_id);
#endif
    }();

    char hostname[1024];
#if defined(PLATFORM_LINUX)
    if (const char* env_hostname = getenv("HOSTNAME")) {
        auto res = fmt::format_to_n(hostname, size(hostname), "{}", env_hostname);
        *res.out = 0;
    } else {
        const auto hn_res = gethostname(hostname, static_cast<int>(size(hostname)));
        if (hn_res != 0) {
            auto res = fmt::format_to_n(hostname, std::size(hostname), "unknown host");
            *res.out = 0;
        }
    }
#else
    DWORD buff_size{ static_cast<DWORD>(std::size(hostname)) };
    const auto hn_res = GetComputerNameExA(ComputerNameDnsFullyQualified, hostname, &buff_size);
    if (hn_res == 0) {
        auto res = fmt::format_to_n(hostname, std::size(hostname), "unknown host");
        *res.out = 0;
    }
#endif

    static constexpr const char* build_file_contents = R"(
#pragma once

namespace xray::build::config {{
static constexpr const char* const commit_hash_str = "{commit_hash_str}";
static constexpr const char* const build_date_time = "{build_time}";
static constexpr const char* const user_info = "{user_info}";
static constexpr const char* const machine_info = "{machine_info}";
}};

)";

    const fs::path build_file{ output_dir / "build.config.hpp" };
    unique_ptr<FILE, FileDeleter> out_file{ fopen(build_file.generic_string().c_str(), "wt") };
    if (!out_file) {
        fmt::print(stderr, "\nFailed to create build config file {}", build_file.generic_string());
        return EXIT_FAILURE;
    }

    chrono::zoned_time zt{ chrono::current_zone()->name(), chrono::system_clock::now() };
    stringstream stime;
    stime << zt;

    fmt::print(out_file.get(),
               build_file_contents,
               fmt::arg("commit_hash_str", commit_sha1.value_or("unknown commit")),
               fmt::arg("build_time", stime.str()),
               fmt::arg("user_info", user_info),
               fmt::arg("machine_info", hostname));

    return EXIT_SUCCESS;
}

int
main(int argc, char** argv)
{
    for (int i = 0; i < argc; ++i) {
        fmt::print(stderr, "\narg[{}] -> {} ", i, argv[i]);
    }

    if (argc == 1) {
        fmt::print(stderr, "\npost-build-tool: nothing to do, exiting ...");
        return EXIT_SUCCESS;
    }

    const fs::path root_path{ argv[1] };

    for (int arg_idx = 2; arg_idx < argc; ++arg_idx) {
        const string_view arg = argv[arg_idx];

        if (arg.empty()) {
            // cmake sends empty strings when genexprs evaluate to false, so ignore them
            continue;
        } else if (const string_view opt{ "--write-build-info" }; arg.starts_with(opt)) {
            if (const int32_t result = write_build_info(root_path); result != EXIT_SUCCESS)
                return result;
        } else if (const string_view opt{ "--write-app-config=" }; arg.starts_with(opt)) {
            const fs::path dest_dir{ arg.substr(opt.length()) };
            if (const int32_t result = write_app_config(root_path / "config/app_config.conf", dest_dir);
                result != EXIT_SUCCESS) {
                return result;
            }
        } else {
            fmt::print("Unhandled argument {}", arg);
            return EXIT_FAILURE;
        }
    }

    return EXIT_SUCCESS;
}
