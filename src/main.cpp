#define GLFW_INCLUDE_VULKAN
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3.h>

#include "app.hpp"

#include "watching.hpp"

using namespace retort;
using namespace retort::utils;

int main(void) {
  auto file_watcher = spawn_file_watcher((const char *)L"E:\\vault");

  auto bootstrapped = bootstrap();
  App app(bootstrapped);

  while (!app.should_close()) {
    app.poll_events();

    while (auto maybe_report = file_watcher.try_pop()) {
      auto report = maybe_report.value();
      if (report.signal != FileWatcherSignal::Modified)
        continue;

      auto file_updated = report.filename;

      auto source = read_file(file_updated.c_str());
      auto compilation_result =
          app.renderer.set_fragment_shader(file_updated.c_str(), source.data());
    }

    app.draw_frame();
  }

  return 0;
}
