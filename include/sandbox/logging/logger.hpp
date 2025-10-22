#pragma once

#include <format>
#include <sandbox/logging/level.hpp>
#include <sandbox/logging/renderer.hpp>
#include <sandbox/logging/writer.hpp>

namespace sandbox::logging {

/**
 * @brief Logger with customizable string rendering and log destination
 */
class logger {
 public:
  static const logger& kColoredLeveledTimestampedStderr() noexcept;
  static const logger& kDefault() noexcept;

 public:
  constexpr logger(logging::level level, logging::renderer renderer,
                   logging::writer writer) noexcept
      : level_(level),
        renderer_(std::move(renderer)),
        writer_(std::move(writer)) {}

 public:
  template <typename... Args>
  constexpr decltype(auto) operator()(logging::level level,
                                      std::format_string<Args...> fmt,
                                      Args&&... args) const noexcept(false) {
    if (level_ <= level) [[likely]] {
      writer_(level,
              renderer_(level, fmt.get(), std::make_format_args(args...)));
    }
  }

 protected:
  logging::level level_;
  logging::renderer renderer_;
  logging::writer writer_;
};

}  // namespace sandbox::logging
