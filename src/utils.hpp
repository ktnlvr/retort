#pragma once

#include <vulkan/vk_enum_string_helper.h>

#include <WinBase.h>
#include <string>

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

#define CHECK_RESULT(expr)                                                     \
  if (!(expr))                                                                 \
  PANIC(#expr)

#define TRY(expr)                                                              \
  if (!(expr))                                                                 \
    return expr;

namespace retort::utils {

std::string get_cwd() {
  auto size = GetCurrentDirectory(0, NULL);
  std::string out(' ', size);
  GetCurrentDirectory(size, out.data());
  return out;
}

std::vector<char> read_file(const char *filename) {
  std::ifstream file(filename, std::ios::ate | std::ios::binary);
  if (!file.is_open()) {
    PANIC("EXPLODE");
  }

  file.seekg(0, std::ios::end);
  size_t file_size = file.tellg();
  std::vector<char> buffer(file_size + 1);
  file.seekg(0);
  file.read(buffer.data(), (std::streamsize)file_size);
  file.close();
  buffer[file_size] = '\0';

  return buffer;
}

} // namespace retort::utils