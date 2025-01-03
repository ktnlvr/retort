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

std::string read_file(const char *filename) {
  std::ifstream file(filename);
  if (!file.is_open()) {
    PANIC("EXPLODE");
  }

  std::stringstream ss;
  ss << file.rdbuf();
  file.close();

  return ss.str();
}

std::optional<std::string> open_file_dialog(GLFWwindow *window) {
  HWND hwnd = glfwGetWin32Window(window);

  const size_t MAX_FILENAME = 1024;

  char filename_buffer[MAX_FILENAME] = {};
  OPENFILENAME open_file = {};
  open_file.lStructSize = sizeof(OPENFILENAME);
  open_file.lpstrFile = filename_buffer;
  open_file.nMaxFile = MAX_FILENAME;
  open_file.hwndOwner = hwnd;
  open_file.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

  auto cwd = utils::get_cwd();
  open_file.lpstrInitialDir = cwd.c_str();

  if (GetOpenFileName(&open_file)) {
    std::string filename(filename_buffer);
    return filename;
  }

  return std::nullopt;
}

} // namespace retort::utils