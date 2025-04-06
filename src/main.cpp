#define GLFW_INCLUDE_VULKAN
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3.h>

#include "app.hpp"

#include "logging.hpp"
#include "watching.hpp"

using namespace retort;
using namespace retort::utils;

int main(void) {
  auto bootstrapped = bootstrap();
  App app(bootstrapped);

  global_logger().log(LogLevel::INFO, "Hello, world!");

  while (!app.should_close()) {
    app.poll_events();
    app.draw_frame();
  }

  return 0;
}
