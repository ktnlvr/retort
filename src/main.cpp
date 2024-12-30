#include "bootstrap.hpp"
#include "renderer.hpp"

using namespace retort;

int main(void) {
  auto bootstrapped = bootstrap();
  Renderer renderer(bootstrapped);

  int pressed = 0;
  while (!glfwWindowShouldClose(renderer.window)) {
    glfwPollEvents();

    auto current_press = glfwGetKey(renderer.window, GLFW_KEY_E) == GLFW_PRESS;
    if (current_press > pressed)
      renderer.set_imgui_enabled(!renderer.is_imgui_enabled);
    pressed = current_press;

    renderer.begin_frame().unwrap();
    ImGui::ShowDemoWindow();
    renderer.end_frame().unwrap();
  }

  return 0;
}
