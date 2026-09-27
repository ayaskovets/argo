#include <argo/awaiter.hpp>
#include <argo/awaiters/continuation_awaiter.hpp>
#include <argo/awaiters/fork_awaiter.hpp>
#include <argo/awaiters/proxy_awaiter.hpp>
#include <argo/awaiters/switch_to_awaiter.hpp>
#include <argo/awaiters/timer_awaiter.hpp>
#include <argo/awaiters/yield_awaiter.hpp>

#include <gtest/gtest.h>

namespace tests::unit {

TEST(awaiter, continuation_awaiter) {
  static_assert(argo::concepts::awaiter<argo::awaiters::continuation_awaiter>);
}

TEST(awaiter, fork_awaiter) {
  static_assert(argo::concepts::awaiter<argo::awaiters::fork_awaiter<int>>);
  static_assert(argo::concepts::awaiter<argo::awaiters::fork_awaiter<void>>);
}

TEST(awaiter, proxy_awaiter) {
  static_assert(argo::concepts::awaiter<argo::awaiters::proxy_awaiter<
                    argo::awaiters::continuation_awaiter, std::identity>>);
}

TEST(awaiter, switch_to_awaiter) {
  static_assert(argo::concepts::awaiter<argo::awaiters::switch_to_awaiter>);
}

TEST(awaiter, timer_awaiter) {
  static_assert(argo::concepts::awaiter<argo::awaiters::timer_awaiter<>>);
}

TEST(awaiter, yield_awaiter) {
  static_assert(argo::concepts::awaiter<argo::awaiters::yield_awaiter>);
}

}  // namespace tests::unit
