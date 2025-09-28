#include "logging/logger.hpp"

#include <gtest/gtest.h>

namespace tests::logging {

TEST(logging_logger, size) {
  static_assert(sizeof(core::logging::logger) == 48);
  static_assert(alignof(core::logging::logger) == 8);
}

TEST(logging_logger, writer) {
  std::string buffer;

  const auto level = core::logging::level::kInfo;
  const auto renderer = core::logging::renderer::kNoop();
  const core::logging::writer writer(
      [&buffer]([[maybe_unused]] core::logging::level level,
                std::string_view message) { buffer = message; });
  const core::logging::logger logger(level, renderer, writer);

  logger(level, "{}", 42);
  EXPECT_EQ(buffer, "42");
}

TEST(logging_logger, noop_level) {
  std::size_t logs_count = 0;

  const auto level = core::logging::level::kWarn;
  const auto renderer = core::logging::renderer::kNoop();
  const core::logging::writer writer(
      [&logs_count]([[maybe_unused]] core::logging::level level,
                    [[maybe_unused]] std::string_view message) {
        logs_count += 1;
      });
  const core::logging::logger logger(level, renderer, writer);

  logger(core::logging::level::kDebug, "message");
  EXPECT_EQ(logs_count, 0);

  logger(core::logging::level::kInfo, "message");
  EXPECT_EQ(logs_count, 0);

  logger(core::logging::level::kWarn, "message");
  EXPECT_EQ(logs_count, 1);

  logger(core::logging::level::kError, "message");
  EXPECT_EQ(logs_count, 2);
}

TEST(logging_logger, renderer) {
  std::string buffer;

  const auto level = core::logging::level::kInfo;
  const core::logging::renderer renderer(
      []([[maybe_unused]] core::logging::level level, std::string_view fmt,
         std::format_args args) {
        return std::vformat(fmt, std::move(args)) +
               std::vformat(fmt, std::move(args));
      });
  const core::logging::writer writer(
      [&buffer]([[maybe_unused]] core::logging::level level,
                std::string_view message) { buffer = message; });
  const core::logging::logger logger(level, renderer, writer);

  logger(core::logging::level::kInfo, "{}", 42);
  EXPECT_EQ(buffer, "4242");
}

}  // namespace tests::logging
