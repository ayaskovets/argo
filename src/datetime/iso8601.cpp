#include <sandbox/datetime/iso8601.hpp>

namespace sandbox::datetime {

std::string to_iso8601(
    std::chrono::system_clock::time_point time_point) noexcept {
  constexpr auto kIso8601Fmt =
      "{:0>4}-{:0>2}-{:0>2}T{:0>2}:{:0>2}:{:0>2}.{:0>6}Z";

  const std::time_t time = std::chrono::system_clock::to_time_t(time_point);
  std::tm tm;
  /* std:: */ gmtime_r(&time, &tm);
  tm.tm_year += 1900;
  tm.tm_mon += 1;
  tm.tm_sec -= (time < 0);
  const auto tm_msec =
      (time_point.time_since_epoch() -
       std::chrono::system_clock::from_time_t(time).time_since_epoch() +
       std::chrono::seconds(time < 0))
          .count();

  return std::format(kIso8601Fmt, tm.tm_year, tm.tm_mon, tm.tm_mday, tm.tm_hour,
                     tm.tm_min, tm.tm_sec, tm_msec);
}

}  // namespace sandbox::datetime
