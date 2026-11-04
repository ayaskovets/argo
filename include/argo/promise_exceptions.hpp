#pragma once

#include <stdexcept>

namespace argo {

/**
 * @brief Thrown when a cancelled promise is resumed or its result is accessed
 */
class promise_cancelled final : public std::runtime_error {
 public:
  constexpr promise_cancelled() noexcept : std::runtime_error("cancelled") {}
};

}  // namespace argo
