#include "bootstrap.hpp"
#include "renderer.hpp"

using namespace retort;

int main(void) {
  auto bootstrapped = bootstrap();
  Renderer renderer(bootstrapped);

  while (!glfwWindowShouldClose(renderer.window)) {
    glfwPollEvents();
    VkResult res = renderer.draw_frame();
    CHECK_VK_ERRC(res);
  }

  return 0;
}
