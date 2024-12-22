#pragma once

#include <iostream>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <VkBootstrap.h>

#include <vulkan/vk_enum_string_helper.h>
#include <vulkan/vulkan.hpp>

#include "utils.hpp"

namespace retort {

struct Bootstrap {
  GLFWwindow *window;
  vkb::Device device;
};

Bootstrap bootstrap() {
  vkb::InstanceBuilder instance_builder;
  auto instance_builder_return = instance_builder.set_app_name("Retort")
                                     .set_engine_name("Retort In-House")
                                     .require_api_version(1, 0, 0)
                                     .use_default_debug_messenger()
                                     .build();
  if (!instance_builder_return) {
    std::cerr << "Failed to create Vulkan instance. Error: "
              << instance_builder_return.error().message() << "\n";
    PANIC("sadge");
  }
  vkb::Instance vkb_instance = instance_builder_return.value();

  vkb::PhysicalDeviceSelector selector{vkb_instance};

  glfwInit();
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  GLFWwindow *window = glfwCreateWindow(640, 480, "Retort", NULL, NULL);

  VkSurfaceKHR surface;
  VkResult err = glfwCreateWindowSurface(vkb_instance, window, NULL, &surface);
  if (err) {
    std::cout << string_VkResult(err) << std::endl;
    PANIC("sadge");
  }

  auto phys_ret = vkb::PhysicalDeviceSelector(vkb_instance)
                      .set_surface(surface)
                      .set_minimum_version(1, 1)
                      .require_dedicated_transfer_queue()
                      .select();
  if (!phys_ret) {
    std::cerr << "Failed to select Vulkan Physical Device. Error: "
              << phys_ret.error().message() << "\n";
    PANIC("sadge");
  }

  vkb::DeviceBuilder device_builder{phys_ret.value()};
  auto dev_ret = device_builder.build();
  if (!dev_ret) {
    std::cerr << "Failed to create Vulkan device. Error: "
              << dev_ret.error().message() << "\n";
    PANIC("sadge");
  }
  vkb::Device vkb_device = dev_ret.value();

  Bootstrap ret;
  ret.window = window;
  ret.device = vkb_device;

  return ret;
}

} // namespace retort
