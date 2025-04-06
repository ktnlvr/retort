#pragma once

#include <GLFW/glfw3native.h>

#include <filesystem>
#include <optional>

#include "logging.hpp"
#include "renderer.hpp"
#include "watching.hpp"

#include <WinBase.h>

namespace retort {

struct AppInteractions {
  std::optional<std::filesystem::path> open_file;
};

struct App {
  bool pressed = 0;

  Renderer renderer;
  FileWatcherPool file_watcher;

  bool show_compilation_logs = false;
  bool show_logs = false;

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
    AppInteractions interactions;

    renderer.begin_frame().unwrap();
    _draw_gui(interactions);
    renderer.end_frame().unwrap();
    _apply_interactions(std::move(interactions));
  }

  void _draw_gui_menu_bar(AppInteractions &interaction) {
    if (ImGui::BeginMainMenuBar()) {
      if (ImGui::BeginMenu("File")) {
        if (ImGui::MenuItem("Open")) {
          auto maybe_filepath = utils::open_file_dialog(renderer.window);
          if (maybe_filepath) {
            auto filename = maybe_filepath.value();
            interaction.open_file = filename;
          }
        }
        ImGui::EndMenu();
      }

      if (ImGui::BeginMenu("View")) {
        if (ImGui::MenuItem("Compilation Logs", nullptr, show_compilation_logs))
          show_compilation_logs = !show_compilation_logs;
        if (ImGui::MenuItem("Debug Logs", nullptr, show_logs))
          show_logs = !show_logs;
        ImGui::EndMenu();
      }

      ImGui::EndMainMenuBar();
    }
  }

  void _draw_gui_logs() {
    if (!show_logs)
      return;

    if (ImGui::Begin("Logs", &show_logs)) {
      auto &logger = global_logger();
      if (ImGui::BeginTable("logs_table", 3, ImGuiTableFlags_Borders)) {
        ImGui::TableSetupColumn("Level", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn("Source", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn("Message");

        for (uint64_t row_i = 0; row_i < logger.messages.size(); row_i++) {
          ImGui::TableNextRow();

          auto &message = logger.messages[row_i];
          ImGui::TableNextColumn();
          ImGui::Text("%s", log_level_to_str(message.level));

          ImGui::TableNextColumn();
          ImGui::Text("%s", message.logger_name.c_str());

          ImGui::TableNextColumn();
          ImGui::TextWrapped("%s", message.message.c_str());
        }

        ImGui::EndTable();
      }
    }

    ImGui::End();
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

  void _draw_gui(AppInteractions &interaction) {
    _draw_gui_menu_bar(interaction);
    _draw_gui_logs();
  }

  void _apply_interactions(AppInteractions &&interaction) {
    if (interaction.open_file)
      add_file(interaction.open_file.value());
  }
};

} // namespace retort
