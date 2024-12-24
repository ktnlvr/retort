#pragma once

#include <stacktrace>
#include <variant>

#include "utils.hpp"

#include <vulkan/vulkan.hpp>

namespace retort {

template <typename Ok, typename Err>
struct [[nodiscard]] Result : private std::variant<Ok, Err> {
  inline Result(Err err) : std::variant<Ok, Err>(err) {}
  inline Result(Ok ok) : std::variant<Ok, Err>(ok) {}

  Ok &unwrap() {
    auto ok_ptr = std::get_if<Ok>(this);
    if (ok_ptr == nullptr)
      PANIC("Failed to unwrap object");

    return *ok_ptr;
  }
};

template <typename Err>
struct [[nodiscard]] Result<void, Err> : private std::optional<Err> {
  inline Result() {}
  inline Result(Err err) : std::optional<Err>(err) {}

  void unwrap() {
    if (this->has_value())
      PANIC("Failed to unwrap object");
  }
};

template <typename Err, Err success>
struct [[nodiscard]] ErrorCodeResult : public Result<void, Err> {
  inline ErrorCodeResult(Err errc) {
    using Base = Result<void, Err>;

    auto base_ptr = (Base *)this;
    if (errc == success) {
      new (base_ptr) Base();
    } else {
      new (base_ptr) Base(errc);
    }
  }
};

using VulkanResult = ErrorCodeResult<VkResult, VkResult::VK_SUCCESS>;

} // namespace retort
