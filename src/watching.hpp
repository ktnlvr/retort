#pragma once

#include <Windows.h>

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
    }

    mutex.unlock();
    return std::nullopt;
  }

  FileWatcherQueue _queue = std::make_shared<FileWatcherQueue::element_type>();
};

#ifdef _WIN32

template <typename F>
void _watch_directory_winapi(const wchar_t *path, F callback) {
  HANDLE directory = CreateFileW(
      path, FILE_LIST_DIRECTORY, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
      OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED, NULL);
  EXPECT(directory != INVALID_HANDLE_VALUE);

  OVERLAPPED polling_overlap = {};
  // TODO(ktnlvr): Check that CreateEvent doesn't fail
  polling_overlap.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

  FILE_NOTIFY_INFORMATION *notify;

  const size_t CHANGE_BUF_SIZE = 1024;
  unsigned char change_buf[CHANGE_BUF_SIZE];

  bool result = ReadDirectoryChangesW(
      directory, change_buf, CHANGE_BUF_SIZE, TRUE,
      FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_LAST_WRITE |
          FILE_NOTIFY_CHANGE_DIR_NAME,
      NULL, &polling_overlap, NULL);

  while (result) {
    DWORD wait_result = WaitForSingleObject(polling_overlap.hEvent, 0);
    if (wait_result == WAIT_OBJECT_0) {
      DWORD bytes_transferred;
      GetOverlappedResult(directory, &polling_overlap, &bytes_transferred,
                          FALSE);

      FILE_NOTIFY_INFORMATION *event = (FILE_NOTIFY_INFORMATION *)change_buf;
      for (;;) {
        DWORD name_length = event->FileNameLength / sizeof(wchar_t);

        switch (event->Action) {
        case FILE_ACTION_MODIFIED: {
          size_t filename_length =
              event->FileNameLength + wcslen(path) + 1 /* the slash */;
          std::wstring full_file_path(filename_length, ' ');
          swprintf(&full_file_path[0], full_file_path.size(), L"%s\\%.*s", path,
                   name_length, event->FileName);

          std::string filepath =
              std::wstring_convert<std::codecvt_utf8<wchar_t>>().to_bytes(
                  full_file_path);

          auto report =
              FileWatcherReport(FileWatcherSignal::Modified, filepath);
          callback(report);
        } break;
        }

        if (event->NextEntryOffset) {
          *((unsigned char **)&event) += event->NextEntryOffset;
        } else {
          break;
        }
      }

      // XXX(ktnlvr): intel only spinlock
      _mm_pause();

      result = ReadDirectoryChangesW(
          directory, change_buf, CHANGE_BUF_SIZE, TRUE,
          FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_LAST_WRITE |
              FILE_NOTIFY_CHANGE_DIR_NAME,
          NULL, &polling_overlap, NULL);
    }
  }
}

#endif

FileWatcher spawn_file_watcher(const char *path) {
  FileWatcher watcher;
  std::thread watcher_thread([=]() {
    _watch_directory_winapi(
        (const wchar_t *)path,
        [=](FileWatcherReport report) { watcher.put(report); });
  });

  // TODO: possible memory leak, patch it up
  watcher_thread.detach();
  return watcher;
}

} // namespace retort
