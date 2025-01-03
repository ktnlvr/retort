#pragma once

#include <stacktrace>
#include <variant>

#include "utils.hpp"

#include <vulkan/vulkan.hpp>

namespace retort {

template <typename Ok, typename... Err>
struct [[nodiscard]] ResultBase : private std::variant<Ok, Err...> {
  using std::variant<Ok, Err...>::variant;

  Ok &unwrap() {
    auto ok_ptr = std::get_if<Ok>(this);
    if (ok_ptr == nullptr)
      PANIC("Failed to unwrap object");
    return *ok_ptr;
  }

  bool is_ok() { return std::get_if<Ok>(this); }

  operator bool() { return this->is_ok(); }
};

template <typename Ok, typename... Err>
struct [[nodiscard]] Result : ResultBase<Ok, Err...> {
  using ResultBase<Ok, Err...>::ResultBase;
};

template <typename Ok, typename Err> struct Result<Ok, Err> : ResultBase<Ok, Err> {
  using ResultBase<Ok, Err>::ResultBase;

  Err &unwrap_err() {
    auto err_ptr = std::get_if<Err>();
    if (err_ptr == nullptr)
      PANIC("Failed to unwrap object");
    return *err_ptr;
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

  bool is_ok() { return !this->has_value(); }

  operator bool() { return this->is_ok(); }
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
