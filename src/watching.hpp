#pragma once

#include <Windows.h>
#include <shlwapi.h>

#include <codecvt>
#include <concepts>
#include <filesystem>
#include <mutex>
#include <optional>
#include <queue>
#include <string>
#include <thread>
#include <tuple>

#include "utils.hpp"

namespace retort {

enum struct FileWatcherSignal {
  Modified,
};

struct FileWatcherReport {
  FileWatcherReport(FileWatcherSignal signal, std::string filename)
      : signal(signal), filename(filename) {}

  FileWatcherSignal signal;
  std::string filename;
};

using FileWatcherQueue =
    std::shared_ptr<std::pair<std::mutex, std::queue<FileWatcherReport>>>;

struct FileWatcher {
  FileWatcher() {}
  ~FileWatcher() {}

  void put(FileWatcherReport report) const {
    auto &mutex = _queue->first;
    mutex.lock();
    _queue->second.push(report);
    mutex.unlock();
  }

  std::optional<FileWatcherReport> try_pop() {
    auto &mutex = _queue->first;
    if (mutex.try_lock()) {
      auto &queue = _queue->second;
      if (queue.size()) {
        FileWatcherReport report = std::move(queue.front());
        queue.pop();
        mutex.unlock();
        return report;
      }
      mutex.unlock();
    }

    return std::nullopt;
  }

  FileWatcherQueue _queue = std::make_shared<FileWatcherQueue::element_type>();
};

#ifdef _WIN32

template <typename F>
void _watch_directory_winapi(const char *path, F callback) {}

#endif

FileWatcher spawn_file_watcher(const char *path) {
  std::string filepath = path;
  FileWatcher watcher;
  std::thread watcher_thread([=]() {
    _watch_directory_winapi(filepath.c_str(), [=](FileWatcherReport report) {
      watcher.put(report);
    });
  });

  // TODO: possible memory leak, patch it up
  watcher_thread.detach();
  return watcher;
}

} // namespace retort
