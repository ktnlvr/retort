#include "bootstrap.hpp"
#include "renderer.hpp"
#include "watching.hpp"

using namespace retort;

int main(void) {
  auto file_watcher = spawn_file_watcher((const char *)L"E:\\vault");

  auto bootstrapped = bootstrap();
  Renderer renderer(bootstrapped);

  bool pressed = 0;
  while (!glfwWindowShouldClose(renderer.window)) {
    glfwPollEvents();

    auto current_press = glfwGetKey(renderer.window, GLFW_KEY_E) == GLFW_PRESS;
    if (current_press > pressed)
      renderer.set_imgui_enabled(!renderer.is_imgui_enabled);
    pressed = current_press;

    while (auto maybe_report = file_watcher.try_pop()) {
      auto report = maybe_report.value();
      if (report.signal != FileWatcherSignal::Modified)
        continue;

      auto file_updated = report.filename;

      auto source = read_file(file_updated.c_str());
      renderer.set_fragment_shader(file_updated.c_str(), source.data());
    }

    renderer.begin_frame().unwrap();
    ImGui::ShowDemoWindow();
    renderer.end_frame().unwrap();
  }

  return 0;
}
