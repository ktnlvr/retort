#pragma once

#include <vulkan/vk_enum_string_helper.h>

#define PANIC(...)                                                             \
  do {                                                                         \
    printf("%s:%d %s", __FILE__, __LINE__, __VA_ARGS__);                       \
    exit(-2);                                                                  \
  } while (0)

#define CHECK_VK_ERRC(expr)                                                    \
  do {                                                                         \
    const VkResult res##__LINE__ = (expr);                                     \
    if (res##__LINE__ != VK_SUCCESS) {                                         \
      PANIC(string_VkResult(res##__LINE__));                                   \
    }                                                                          \
  } while (0)