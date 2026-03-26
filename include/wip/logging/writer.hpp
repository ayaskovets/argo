#pragma once

#include <wip/logging/level.hpp>

#include <functional>
#include <string_view>

namespace wip::logging {

/**
 * @brief Log destination
 */
class writer final {
 public:
  static const writer& kStdout() noexcept;
  static const writer& kStderr() noexcept;
  static const writer& kNoop() noexcept;
  static const writer& kDefault() noexcept;

 private:
  using write_t =
      std::function<void(logging::level level, std::string_view message)>;

 public:
  constexpr explicit writer(write_t write) noexcept
      : write_(std::move(write)) {}

 public:
  constexpr void operator()(logging::level level,
                            std::string_view message) const noexcept(false) {
    write_(level, message);
  }

 private:
  write_t write_;
};

}  // namespace wip::logging
