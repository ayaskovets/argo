#include <argo/channel.hpp>

#include <flow/mpmc_queue.hpp>
#include <flow/mpsc_queue.hpp>
#include <flow/spmc_queue.hpp>
#include <flow/spsc_queue.hpp>
#include <flow/unbounded_spsc_queue.hpp>
#include <gtest/gtest.h>

namespace tests::unit {

TEST(channel, flow) {
  static_assert(argo::concepts::channel<flow::mpmc_queue<int>>);
  static_assert(argo::concepts::noexcept_channel<flow::mpmc_queue<int>>);

  static_assert(argo::concepts::channel<flow::mpsc_queue<int>>);
  static_assert(argo::concepts::noexcept_channel<flow::mpsc_queue<int>>);

  static_assert(argo::concepts::channel<flow::spmc_queue<int>>);
  static_assert(argo::concepts::noexcept_channel<flow::spmc_queue<int>>);

  static_assert(argo::concepts::channel<flow::spsc_queue<int>>);
  static_assert(argo::concepts::noexcept_channel<flow::spsc_queue<int>>);

  static_assert(argo::concepts::channel<flow::unbounded_spsc_queue<int>>);
  static_assert(
      !argo::concepts::noexcept_channel<flow::unbounded_spsc_queue<int>>);
}

}  // namespace tests::unit
