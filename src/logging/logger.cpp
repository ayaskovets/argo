#include <sandbox/logging/logger.hpp>

namespace sandbox::logging {

const logger &logger::kColoredLeveledTimestampedStderr() noexcept {
  static const logger logger(logging::level::kInfo,
                             logging::renderer::kDefault(),
                             logging::writer::kDefault());
  return logger;
}

const logger &logger::kDefault() noexcept {
  return kColoredLeveledTimestampedStderr();
}

}  // namespace sandbox::logging
