#include "xray/base/file_system_watcher.hpp"

#if defined(XRAY_OS_IS_POSIX_FAMILY)
#include <itlib/small_vector.hpp>
#include <pipes/pipes.hpp>
#endif

#include "xray/base/logger.hpp"

namespace xray::base {

FileSystemWatcher::~FileSystemWatcher()
{
#if defined(XRAY_OS_IS_POSIX_FAMILY)
    _fs_watched_paths >>= pipes::for_each(
        [fd = raw_handle(_inotify_handle)](const std::pair<std::filesystem::path, int32_t>& path_watch_pair) {
            syscall_wrapper(inotify_rm_watch, fd, path_watch_pair.second);
        });
#endif
}

FileSystemWatcher::FileSystemWatcher(const std::filesystem::path& p, FileSystemObserverDelegate observer)
{
    add_watch(p, observer);
}

void
FileSystemWatcher::add_watch(const std::filesystem::path& path,
                             FileSystemObserverDelegate observer,
                             const bool recursive)
{
#if defined(XRAY_OS_IS_POSIX_FAMILY)
    const uint32_t watch_flags = IN_CLOSE_WRITE | IN_MODIFY;

    const int32_t watch_descriptor =
        syscall_wrapper(inotify_add_watch, raw_handle(_inotify_handle), path.c_str(), IN_CLOSE_WRITE | IN_MODIFY);

    if (watch_descriptor == -1)
        return;

    _fs_watched_paths.emplace(path.string(), watch_descriptor);
    _fs_watch_instances.emplace(watch_descriptor, FsWatchInstanceEntry{ path, observer });
    XR_LOG_INFO("Adding path to watch: {}", path.c_str());

    namespace fs = std::filesystem;
    if (!fs::is_directory(path) || !recursive)
        return;

    for (const fs::directory_entry& dir_entry : fs::recursive_directory_iterator(path)) {
        if (!fs::is_directory(dir_entry.path()))
            continue;

        const int32_t watch_desc =
            syscall_wrapper(inotify_add_watch, raw_handle(_inotify_handle), dir_entry.path().c_str(), watch_flags);
        if (watch_desc == -1) {
            XR_LOG_ERR("inotify_add_watch failed on directory {}, errno {}", dir_entry.path().c_str(), errno);
            continue;
        }

        XR_LOG_INFO("Adding path to watch: {}", dir_entry.path().c_str());
        _fs_watched_paths[dir_entry.path()] = watch_desc;
        _fs_watch_instances[watch_desc] = FsWatchInstanceEntry{ dir_entry.path(), observer };
    }
#endif
}

void
FileSystemWatcher::poll()
{
#if defined(XRAY_OS_IS_POSIX_FAMILY)
    itlib::small_vector<inotify_event, 8> fs_events{ 8 };

    alignas(inotify_event) uint8_t fs_events_buffer[(sizeof(inotify_event) + NAME_MAX + 1) * 4];

    const iovec iovec_data = { .iov_base = fs_events_buffer, .iov_len = sizeof(fs_events_buffer) };

    for (size_t offset = 0;;) {
        const ssize_t bytes_read = syscall_wrapper(readv, raw_handle(_inotify_handle), &iovec_data, 1);

        if (bytes_read == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            return;
        }

        if (bytes_read <= 0) {
            XR_LOG_ERR("inotify read failure {}", errno);
            return;
        }

        const inotify_event* event = reinterpret_cast<const inotify_event*>(&fs_events_buffer[0] + offset);
        offset += event->len + sizeof(inotify_event);

        if (auto watch_entry = _fs_watch_instances.find(event->wd); watch_entry != std::cend(_fs_watch_instances)) {
            watch_entry->second.observer(FileModifiedEvent{ watch_entry->second.path / event->name });
        }
    }
#endif
}

}
