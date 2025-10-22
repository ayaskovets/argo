#include <sandbox/logging/logger.hpp>

#include <gtest/gtest.h>

namespace tests::logging {

TEST(logging_logger, size) {
  static_assert(sizeof(sandbox::logging::logger) == 48);
  static_assert(alignof(sandbox::logging::logger) == 8);
}

TEST(logging_logger, writer) {
  std::string buffer;

  const auto level = sandbox::logging::level::kInfo;
  const auto renderer = sandbox::logging::renderer::kNoop();
  const sandbox::logging::writer writer(
      [&buffer]([[maybe_unused]] sandbox::logging::level level,
                std::string_view message) { buffer = message; });
  const sandbox::logging::logger logger(level, renderer, writer);

  logger(level, "{}", 42);
  EXPECT_EQ(buffer, "42");
}

TEST(logging_logger, noop_level) {
  std::size_t logs_count = 0;

  const auto level = sandbox::logging::level::kWarn;
  const auto renderer = sandbox::logging::renderer::kNoop();
  const sandbox::logging::writer writer(
      [&logs_count]([[maybe_unused]] sandbox::logging::level level,
                    [[maybe_unused]] std::string_view message) {
        logs_count += 1;
      });
  const sandbox::logging::logger logger(level, renderer, writer);

  logger(sandbox::logging::level::kDebug, "message");
  EXPECT_EQ(logs_count, 0);

  logger(sandbox::logging::level::kInfo, "message");
  EXPECT_EQ(logs_count, 0);

  logger(sandbox::logging::level::kWarn, "message");
  EXPECT_EQ(logs_count, 1);

  logger(sandbox::logging::level::kError, "message");
  EXPECT_EQ(logs_count, 2);
}

TEST(logging_logger, renderer) {
  std::string buffer;

  const auto level = sandbox::logging::level::kInfo;
  const sandbox::logging::renderer renderer(
      []([[maybe_unused]] sandbox::logging::level level, std::string_view fmt,
         std::format_args args) {
        return std::vformat(fmt, std::move(args)) +
               std::vformat(fmt, std::move(args));
      });
  const sandbox::logging::writer writer(
      [&buffer]([[maybe_unused]] sandbox::logging::level level,
                std::string_view message) { buffer = message; });
  const sandbox::logging::logger logger(level, renderer, writer);

  logger(sandbox::logging::level::kInfo, "{}", 42);
  EXPECT_EQ(buffer, "4242");
}

}  // namespace tests::logging
