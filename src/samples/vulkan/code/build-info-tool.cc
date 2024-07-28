#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <memory>
#include <chrono>
#include <sstream>
#include <string>
#include <string_view>

#include <fmt/core.h>
#include <fmt/chrono.h>

#include <git2.h>

#include <pwd.h>
#include <unistd.h>

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
    root_win = "c:/games/xray"; 
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

    char commit_sha1[GIT_OID_MAX_SIZE + 1];
    git_oid_tostr(commit_sha1, size(commit_sha1), &oid_parent_commit);

    // if (git_commit* last_commit{nullptr}; git_commit_lookup(&last_commit, repo, &oid_parent_commit) == 0) {
    //     const git_time_t last_commit_time = git_commit_time(last_commit);
    //     const int32_t commit_time_offset = git_commit_time_offset(last_commit);
    // }
    //

    const uid_t user_id = getuid();

    const string user_info = [user_id]() {
        vector<uint8_t> temp_buffer;
        const auto bsize = sysconf(_SC_GETPW_R_SIZE_MAX);
        temp_buffer.resize(bsize > 0 ? static_cast<size_t>(bsize) : 16384);

        passwd user_data;
        passwd* result{};
        getpwuid_r(user_id, &user_data, reinterpret_cast<char*>(temp_buffer.data()), temp_buffer.size(), &result);
        return fmt::format(
            "{} ({}, uid {})", result ? result->pw_name : "unknown", result ? result->pw_gecos : "unknown", user_id);
    }();

    char hostname[HOST_NAME_MAX];
    if (const char* env_hostname = getenv("HOSTNAME")) {
        snprintf(hostname, size(hostname), "%s", env_hostname);
    } else {
        if (gethostname(hostname, size(hostname)) != 0) {
            snprintf(hostname, size(hostname), "unknown host");
        }
    }

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
               fmt::arg("commit_hash_str", commit_sha1),
               fmt::arg("build_time", stime.str()),
               fmt::arg("user_info", user_info),
               fmt::arg("machine_info", hostname));

    return EXIT_SUCCESS;
}

int
main(int argc, char** argv)
{
    // for (int i = 0; i < argc; ++i) {
    //     fmt::print(stderr, "\narg[{}] -> {} ", i, argv[i]);
    // }

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
