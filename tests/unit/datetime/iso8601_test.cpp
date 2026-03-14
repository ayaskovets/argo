#include <wip/async/channel.hpp>
#include <wip/datetime/iso8601.hpp>

#include <gtest/gtest.h>

namespace tests::datetime {

namespace datetime = wip::datetime;

TEST(datetime_iso8601, to_iso8601) {
  using namespace std::chrono_literals;

  EXPECT_EQ(datetime::to_iso8601(
                std::chrono::sys_days{std::chrono::January / 9 / 2014} + 12h +
                35min + 34s + 125ms + 322us),
            "2014-01-09T12:35:34.125322Z");
  EXPECT_EQ(datetime::to_iso8601(
                std::chrono::sys_days{std::chrono::January / 9 / 1300} + 12h +
                35min + 34s + 125ms + 322us),
            "1300-01-09T12:35:34.125322Z");
  EXPECT_EQ(datetime::to_iso8601(
                std::chrono::sys_days{std::chrono::January / 9 / 3235} + 12h +
                35min + 34s + 125ms + 322us),
            "3235-01-09T12:35:34.125322Z");
}

}  // namespace tests::datetime
