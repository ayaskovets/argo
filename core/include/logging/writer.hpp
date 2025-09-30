#pragma once

#include <functional>
#include <string_view>

#include "logging/level.hpp"

namespace core::logging {

/**
 * @brief Log destination
 */
class writer final {
 public:
  static const writer& kStdout();
  static const writer& kStderr();
  static const writer& kNoop();
  static const writer& kDefault();

 private:
  using write_t =
      std::function<void(logging::level level, std::string_view message)>;

 public:
  constexpr explicit writer(write_t write) noexcept
      : write_(std::move(write)) {}

 public:
  constexpr void operator()(logging::level level,
                            std::string_view message) const {
    write_(level, message);
  }

 private:
  write_t write_;
};

}  // namespace core::logging
