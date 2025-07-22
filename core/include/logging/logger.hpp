#pragma once

#include <format>

#include "logging/level.hpp"
#include "logging/renderer.hpp"
#include "logging/writer.hpp"

namespace core::logging {

/**
 * @brief Logger with customizable string rendering and log destination
 */
class logger {
 public:
  static const logger& kColoredLeveledTimestampedStderr();
  static const logger& kDefault();

 public:
  logger(logging::level level, logging::renderer renderer,
         logging::writer writer) noexcept
      : level_(level),
        renderer_(std::move(renderer)),
        writer_(std::move(writer)) {}

 public:
  template <typename... Args>
  constexpr decltype(auto) operator()(logging::level level,
                                      std::format_string<Args...> fmt,
                                      Args&&... args) const {
    if (level_ <= level) [[likely]] {
      writer_(level,
              renderer_(level, fmt.get(),
                        std::make_format_args(std::forward<Args>(args)...)));
    }
  }

 protected:
  logging::level level_;
  logging::renderer renderer_;
  logging::writer writer_;
};

}  // namespace core::logging
