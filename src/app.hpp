#pragma once

#include <GLFW/glfw3native.h>

#include "renderer.hpp"

#include <WinBase.h>

namespace retort {

struct App {
  App(Bootstrap bootstrap) : renderer(bootstrap) {}

  bool should_close() { return glfwWindowShouldClose(renderer.window); }

  void poll_events() {
    glfwPollEvents();
    auto current_press =
        glfwGetKey(renderer.window, GLFW_KEY_ESCAPE) == GLFW_PRESS;
    if (current_press > pressed)
      renderer.set_imgui_enabled(!renderer.is_imgui_enabled);
    pressed = current_press;
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
          auto shader = open_fragment_shader_dialog();
          std::cout << shader.value() << std::endl;
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

  void _draw_gui() { _draw_gui_menu_bar(); }

  std::optional<std::string> open_fragment_shader_dialog() {
    HWND hwnd = glfwGetWin32Window(renderer.window);

    const size_t MAX_FILENAME = 1024;

    char filename_buffer[MAX_FILENAME] = {};
    OPENFILENAME open_file = {};
    open_file.lStructSize = sizeof(OPENFILENAME);
    open_file.lpstrFile = filename_buffer;
    open_file.nMaxFile = MAX_FILENAME;
    open_file.hwndOwner = hwnd;
    open_file.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

    auto cwd = utils::get_cwd();
    open_file.lpstrInitialDir = cwd.c_str();

    if (GetOpenFileName(&open_file)) {
      std::string filename(filename_buffer);
      return filename;
    }

    return std::nullopt;
  }

  void _update_fragment_shader_from_file(std::string filepath) {}

  bool pressed = 0;
  Renderer renderer;

  bool show_compilation_logs = false;
};

} // namespace retort
