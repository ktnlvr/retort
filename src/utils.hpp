#pragma once

#include <vulkan/vk_enum_string_helper.h>

#define PANIC(str)                                                             \
  do {                                                                         \
    printf("%s:%d %s", __FILE__, __LINE__, str);                               \
    exit(-2);                                                                  \
  } while (0)

#define EXPECT(expr)                                                           \
  do {                                                                         \
    if (!(expr))                                                               \
      PANIC(#expr);                                                            \
  } while (0)

#define CHECK_VK_ERRC(expr)                                                    \
  do {                                                                         \
    const VkResult res##__LINE__ = (expr);                                     \
    if (res##__LINE__ != VK_SUCCESS) {                                         \
      PANIC(string_VkResult(res##__LINE__));                                   \
    }                                                                          \
  } while (0)