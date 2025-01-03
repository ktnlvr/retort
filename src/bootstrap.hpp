#pragma once

#include <iostream>

#include <VkBootstrap.h>

#include <vulkan/vk_enum_string_helper.h>
#include <vulkan/vulkan.hpp>

#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>
#include <imgui.h>

#include "utils.hpp"

namespace retort {

struct Bootstrap {
  GLFWwindow *window;
  vkb::Device device;
  vkb::Instance instance;
  vkb::PhysicalDevice physical_device;
};

Bootstrap bootstrap() {
  EXPECT(glfwInit());
  EXPECT(glfwVulkanSupported());

  std::vector<const char *> vulkan_extensions;

  uint32_t glfw_extension_count = 0;
  const char **glfw_extensions =
      glfwGetRequiredInstanceExtensions(&glfw_extension_count);
  for (uint32_t i = 0; i < glfw_extension_count; i++)
    vulkan_extensions.push_back(glfw_extensions[i]);

  vkb::InstanceBuilder instance_builder;
  auto instance_builder_return = instance_builder.set_app_name("Retort")
                                     .set_engine_name("Retort In-House")
                                     .require_api_version(1, 2, 0)
                                     .use_default_debug_messenger()
                                     .enable_extensions(vulkan_extensions)
                                     .enable_validation_layers()
                                     .build();
  if (!instance_builder_return) {
    std::cerr << "Failed to create Vulkan instance. Error: "
              << instance_builder_return.error().message() << "\n";
    PANIC("sadge");
  }
  vkb::Instance vkb_instance = instance_builder_return.value();

  vkb::PhysicalDeviceSelector selector{vkb_instance};

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
                      .set_minimum_version(1, 2)
                      .require_dedicated_transfer_queue()
                      .select();
  if (!phys_ret) {
    std::cerr << "Failed to select Vulkan Physical Device. Error: "
              << phys_ret.error().message() << "\n";
    PANIC("sadge");
  }

  auto vkb_physical = phys_ret.value();

  vkb::DeviceBuilder device_builder{vkb_physical};
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
  ret.instance = vkb_instance;
  ret.physical_device = vkb_physical;

  return ret;
}

} // namespace retort
