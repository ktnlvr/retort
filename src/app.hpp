#pragma once

#include <GLFW/glfw3native.h>

#include "renderer.hpp"
#include "watching.hpp"

#include <WinBase.h>

namespace retort {

struct App {
  bool pressed = 0;
  Renderer renderer;
  FileWatcherPool file_watcher;

  bool show_compilation_logs = false;

  App(Bootstrap bootstrap) : renderer(bootstrap) {
    glfwSetWindowUserPointer(bootstrap.window, this);
    glfwSetDropCallback(bootstrap.window, [](GLFWwindow *window, int path_count,
                                             const char **paths) {
      EXPECT(path_count == 1);
      App *self = (App *)glfwGetWindowUserPointer(window);
      for (int i = 0; i < path_count; i++) {
        const char *path_cstr = paths[i];
        auto path = std::filesystem::path(path_cstr);
        self->add_file(path);
      }
    });
  }

  bool should_close() { return glfwWindowShouldClose(renderer.window); }

  void poll_events() {
    glfwPollEvents();
    auto current_press =
        glfwGetKey(renderer.window, GLFW_KEY_ESCAPE) == GLFW_PRESS;
    if (current_press > pressed)
      renderer.set_imgui_enabled(!renderer.is_imgui_enabled);
    pressed = current_press;

    auto changed = file_watcher.poll_files();
    if (changed.size()) {
      auto [_, filepath] = changed[0];
      _set_focused_shader_file(filepath);
    }
  }

  void draw_frame() {
    renderer.begin_frame().unwrap();
    _draw_gui();
    renderer.end_frame().unwrap();
  }

  void _draw_gui_menu_bar() {
    if (ImGui::BeginMainMenuBar()) {
      if (ImGui::BeginMenu("File")) {
        if (ImGui::MenuItem("Open")) {
          auto maybe_filepath = utils::open_file_dialog(renderer.window);
          if (maybe_filepath) {
            auto filename = maybe_filepath.value();
            add_file(filename);
          }
        }
        ImGui::EndMenu();
      }

      if (ImGui::BeginMenu("View")) {
        if (ImGui::MenuItem("Compilation Logs", nullptr, nullptr,
                            show_compilation_logs))
          show_compilation_logs = !show_compilation_logs;
        ImGui::EndMenu();
      }

      ImGui::EndMainMenuBar();
    }
  }

  void add_file(std::filesystem::path file) {
    _set_focused_shader_file(file);
    file_watcher.watch_file(file);
  }

  void _set_focused_shader_file(std::filesystem::path path) {
    auto path_str = path.string();
    auto source = utils::read_file(path_str.c_str());
    renderer.set_fragment_shader(path_str.c_str(), source.c_str());
  }

  void _draw_gui() { _draw_gui_menu_bar(); }
};

} // namespace retort
