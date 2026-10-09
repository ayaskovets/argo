#include <argo/awaiter.hpp>
#include <argo/awaiters/yield_awaiter.hpp>

#include <gtest/gtest.h>

namespace tests::unit {

TEST(awaiter, yield_awaiter) {
  static_assert(argo::concepts::awaiter<argo::awaiters::yield>);
}

}  // namespace tests::unit
