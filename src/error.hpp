#pragma once

#include <stacktrace>
#include <variant>

#include "utils.hpp"

namespace retort {

struct _ResultBase {
  _ResultBase() {}
};

template <typename Ok, typename Err>
struct [[nodiscard]] Result : std::variant<Ok, Err>, _ResultBase {
  inline Result(Err err) : std::variant<Ok, Err>(err) {}
  inline Result(Ok ok) : std::variant<Ok, Err>(ok) {}
};

template <typename Err>
struct [[nodiscard]] Result<void, Err> : std::optional<Err>, _ResultBase {
  inline Result() {}
  inline Result(Err err) : std::optional<Err>(err) {}
};

template <typename T = void> using VulkanResult = Result<T, VkResult>;

} // namespace retort
