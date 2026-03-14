#include <wip/logging/level.hpp>
#include <wip/logging/writer.hpp>

#include <cstdio>
#include <format>

namespace wip::logging {

const writer& writer::kStdout() noexcept {
  static const writer writer(
      []([[maybe_unused]] logging::level level, std::string_view message) {
        if (std::fputs(message.data(), stdout) == EOF) [[unlikely]] {
          throw std::runtime_error(std::format(
              "fputs(stdout) failed: {}", std::strerror(std::ferror(stdout))));
        }
      });
  return writer;
}

const writer& writer::kStderr() noexcept {
  static const writer writer(
      []([[maybe_unused]] logging::level level, std::string_view message) {
        if (std::fputs(message.data(), stderr) == EOF) [[unlikely]] {
          throw std::runtime_error(std::format(
              "fputs(stderr) failed: {}", std::strerror(std::ferror(stderr))));
        }
      });
  return writer;
}

const writer& writer::kNoop() noexcept {
  static const writer writer([]([[maybe_unused]] logging::level level,
                                [[maybe_unused]] std::string_view message) {});
  return writer;
}

const writer& writer::kDefault() noexcept { return kStderr(); }

}  // namespace wip::logging
