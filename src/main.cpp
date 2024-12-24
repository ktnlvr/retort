#include "bootstrap.hpp"
#include "renderer.hpp"

using namespace retort;

int main(void) {
  auto bootstrapped = bootstrap();
  Renderer renderer(bootstrapped);

  while (!glfwWindowShouldClose(renderer.window)) {
    glfwPollEvents();
    renderer.begin_frame().unwrap();
    ImGui::ShowDemoWindow();
    renderer.end_frame().unwrap();
  }

  return 0;
}
