#include "bootstrap.hpp"
#include "renderer.hpp"

using namespace retort;

int main(void) {
  auto bootstrapped = bootstrap();
  Renderer renderer(bootstrapped);

  while (!glfwWindowShouldClose(renderer.window)) {
    glfwPollEvents();
    auto res = renderer.draw_frame();
  }

  return 0;
}
