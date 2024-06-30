#pragma once

#include "xray/xray.hpp"

#include <cstdint>
#include <filesystem>
#include <type_traits>
#include <unordered_map>
#include <variant>

#if defined(XRAY_OS_IS_POSIX_FAMILY)
#include <sys/inotify.h>
#include <sys/uio.h>
#include <unistd.h>

#include "xray/base/syscall_wrapper.hpp"

#endif

#include "xray/base/delegate.hpp"
#include "xray/base/unique_handle.hpp"

namespace xray::base {

#if defined(XRAY_OS_IS_POSIX_FAMILY)
class InotifyHandleTraits
{
  public:
    using handle_type = int32_t;

    static constexpr handle_type null() noexcept { return -1; }

    static bool is_null(const handle_type h) noexcept { return h == -1; }

    static void destroy(const handle_type h) noexcept { syscall_wrapper(close, h); }
};
#endif

struct FileModifiedEvent
{
    std::filesystem::path file;
};

using FileSystemEvent = std::variant<FileModifiedEvent>;

class FileSystemWatcher
{
  public:
    using FileSystemObserverDelegate = cpp::delegate<void(const FileSystemEvent&)>;

    FileSystemWatcher() = default;
    FileSystemWatcher(const std::filesystem::path& p, FileSystemObserverDelegate observer);

    ~FileSystemWatcher();

    void poll();
    void add_watch(const std::filesystem::path& path, FileSystemObserverDelegate observer, const bool recursive = true);

  private:
#if defined(XRAY_OS_IS_POSIX_FAMILY)
    struct FsWatchInstanceEntry
    {
        std::filesystem::path path;
        fast_delegate<void(const FileSystemEvent&)> observer;
    };

    xray::base::unique_handle<InotifyHandleTraits> _inotify_handle{ syscall_wrapper(inotify_init1, IN_NONBLOCK) };
    std::unordered_map<std::filesystem::path, int32_t> _fs_watched_paths;
    std::unordered_map<int32_t, FsWatchInstanceEntry> _fs_watch_instances;
#endif
};

} // namespace xtay::base
