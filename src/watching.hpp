#pragma once

#include <Windows.h>
#include <shlwapi.h>

#include <filesystem>
#include <map>

#include "utils.hpp"

namespace retort {

using WatchedFileId = uint16_t;

struct FileWatcherPool {
  using PollReturn =
      std::vector<std::tuple<WatchedFileId, std::filesystem::path>>;

  WatchedFileId watch_file(std::filesystem::path file) {
    _filepaths[0] = file;
    return 0;
  }

  void forget_file(WatchedFileId id) {}

  std::unordered_map<WatchedFileId, std::filesystem::path> _filepaths;
  std::unordered_map<WatchedFileId, std::filesystem::file_time_type> _ts;

  PollReturn poll_files() {
    // TODO: replace with a better notification system
    PollReturn changed;

    for (auto &[id, path] : _filepaths) {
      auto t1 = std::filesystem::last_write_time(path);
      if (_ts.contains(id)) {
        auto t0 = _ts[id];
        if (t0 != t1) {
          auto path_str = path.string();
          changed.push_back(std::make_tuple(id, path));
        }
      }
      _ts[id] = t1;
    }

    return changed;
  }
};

} // namespace retort
