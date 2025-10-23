#include <sandbox/logging/logger.hpp>

#include <gtest/gtest.h>

namespace tests::logging {

namespace logging = sandbox::logging;

TEST(logging_logger, size) {
  static_assert(sizeof(logging::logger) == 48);
  static_assert(alignof(logging::logger) == 8);
}

TEST(logging_logger, writer) {
  std::string buffer;

  const auto level = logging::level::kInfo;
  const auto renderer = logging::renderer::kNoop();
  const logging::writer writer(
      [&buffer]([[maybe_unused]] logging::level level,
                std::string_view message) { buffer = message; });
  const logging::logger logger(level, renderer, writer);

  logger(level, "{}", 42);
  EXPECT_EQ(buffer, "42");
}

TEST(logging_logger, noop_level) {
  std::size_t logs_count = 0;

  const auto level = logging::level::kWarn;
  const auto renderer = logging::renderer::kNoop();
  const logging::writer writer(
      [&logs_count]([[maybe_unused]] logging::level level,
                    [[maybe_unused]] std::string_view message) {
        logs_count += 1;
      });
  const logging::logger logger(level, renderer, writer);

  logger(logging::level::kDebug, "message");
  EXPECT_EQ(logs_count, 0);

  logger(logging::level::kInfo, "message");
  EXPECT_EQ(logs_count, 0);

  logger(logging::level::kWarn, "message");
  EXPECT_EQ(logs_count, 1);

  logger(logging::level::kError, "message");
  EXPECT_EQ(logs_count, 2);
}

TEST(logging_logger, renderer) {
  std::string buffer;

  const auto level = logging::level::kInfo;
  const logging::renderer renderer([]([[maybe_unused]] logging::level level,
                                      std::string_view fmt,
                                      std::format_args args) {
    return std::vformat(fmt, std::move(args)) +
           std::vformat(fmt, std::move(args));
  });
  const logging::writer writer(
      [&buffer]([[maybe_unused]] logging::level level,
                std::string_view message) { buffer = message; });
  const logging::logger logger(level, renderer, writer);

  logger(logging::level::kInfo, "{}", 42);
  EXPECT_EQ(buffer, "4242");
}

}  // namespace tests::logging
